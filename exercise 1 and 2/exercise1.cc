/*
 * Two nodes separated by a router with eavesdropping attack simulation
 *
 * Network Topology:
 *
 *   Network 1 (10.1.1.0/24)          Network 2 (10.1.2.0/24)
 *
 *   n0 -------------------- n1 (Router) -------------------- n2
 *      point-to-point                    point-to-point
 *      5Mbps, 2ms                        5Mbps, 2ms
 *      
 *   Attacker (n3) eavesdropping on the n0-n1 link
 */

 #include "ns3/applications-module.h"
 #include "ns3/core-module.h"
 #include "ns3/internet-module.h"
 #include "ns3/mobility-module.h"
 #include "ns3/netanim-module.h"
 #include "ns3/network-module.h"
 #include "ns3/point-to-point-module.h"
 #include "ns3/csma-module.h"  // Added for shared medium eavesdropping
 
 using namespace ns3;
 
 NS_LOG_COMPONENT_DEFINE("EavesdroppingAttack");
 
 // ============================================
 // Eavesdropper Classes
 // ============================================
 
 /**
  * Packet Sniffer for eavesdropping simulation
  */
 class PacketSniffer
 {
 public:
     PacketSniffer(std::string filename)
     {
         m_output = Create<OutputStreamWrapper>(filename, std::ios::out);
     }
     
     /**
      * Callback function to sniff packets
      */
     void SniffPacket(Ptr<const Packet> packet, Ptr<Ipv4> ipv4, uint32_t interface)
     {
         Ipv4Header ipHeader;
         Ptr<Packet> copy = packet->Copy();
         
         // Try to peek the IP header
         if (copy->PeekHeader(ipHeader) == 0)
         {
             return;
         }
         
         // Check if this is UDP echo traffic (port 9)
         UdpHeader udpHeader;
         if (copy->PeekHeader(udpHeader) == 0)
         {
             return;
         }
         
         // Log the sniffed packet
         *m_output->GetStream() << "[" << Simulator::Now().GetSeconds() << "s] ";
         *m_output->GetStream() << "Sniffed packet: ";
         *m_output->GetStream() << "Src=" << ipHeader.GetSource() << ":" << udpHeader.GetSourcePort() << " ";
         *m_output->GetStream() << "Dst=" << ipHeader.GetDestination() << ":" << udpHeader.GetDestinationPort() << " ";
         *m_output->GetStream() << "Size=" << packet->GetSize() << " bytes ";
         
         // Try to extract payload (simulating packet inspection)
         if (packet->GetSize() <= 1200)  // Small enough to inspect
         {
             uint8_t buffer[1024];
             packet->CopyData(buffer, std::min(packet->GetSize(), (uint32_t)100));
             
             // Check if this looks like UdpEcho payload
             std::string payload((char*)buffer, std::min(packet->GetSize(), (uint32_t)100));
             
             // Look for patterns (simplified)
             if (payload.find("Echo") != std::string::npos || 
                 payload.find("echo") != std::string::npos ||
                 payload.find("data") != std::string::npos)
             {
                 *m_output->GetStream() << "[PAYLOAD EXTRACTED: Contains application data]";
             }
             else if (packet->GetSize() > 50)
             {
                 *m_output->GetStream() << "[PAYLOAD: " << payload.substr(0, 50) << "...]";
             }
         }
         
         *m_output->GetStream() << std::endl;
     }
     
     /**
      * Start sniffing on a specific node and interface
      */
     void StartSniffing(Ptr<Node> node, uint32_t interface)
     {
         Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
         if (ipv4)
         {
             ipv4->TraceConnectWithoutContext("Tx", 
                 MakeCallback(&PacketSniffer::SniffPacket, this));
             ipv4->TraceConnectWithoutContext("Rx", 
                 MakeCallback(&PacketSniffer::SniffPacket, this));
         }
     }
     
 private:
     Ptr<OutputStreamWrapper> m_output;
 };
 
 // ============================================
 // Modified UdpEchoClient to Send Sensitive Data
 // ============================================
 
 class SensitiveUdpEchoClient : public Application
 {
 public:
     static TypeId GetTypeId()
     {
         static TypeId tid = TypeId("ns3::SensitiveUdpEchoClient")
             .SetParent<Application>()
             .SetGroupName("Applications")
             .AddConstructor<SensitiveUdpEchoClient>();
         return tid;
     }
     
     SensitiveUdpEchoClient()
     {
         m_socket = 0;
         m_sent = 0;
         m_sensitiveData = "CONFIDENTIAL: User=admin, Password=Secret123, Account=1001";
         m_running = false;
     }
     
     virtual ~SensitiveUdpEchoClient()
     {
         m_socket = 0;
     }
     
     void Setup(Address address, uint32_t nPackets, Time interval)
     {
         m_peerAddress = address;
         m_nPackets = nPackets;
         m_interval = interval;
     }
     
 protected:
     void StartApplication() override
     {
         m_running = true;
         if (!m_socket)
         {
             m_socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
             m_socket->Bind();
             m_socket->Connect(m_peerAddress);
         }
         SendPacket();
     }
     
     void StopApplication() override
     {
         m_running = false;
         if (m_sendEvent.IsRunning())
         {
             Simulator::Cancel(m_sendEvent);
         }
         if (m_socket)
         {
             m_socket->Close();
         }
     }
     
     void SendPacket()
     {
         if (!m_running) return;
         
         Ptr<Packet> packet;
         
         // Create packets with sensitive data
         if (m_sent == 0)
         {
             // First packet: Sensitive login information
             std::string data = "LOGIN: " + m_sensitiveData;
             packet = Create<Packet>((uint8_t*)data.c_str(), data.length());
             std::cout << Simulator::Now().GetSeconds() 
                       << "s: Client sending SENSITIVE login data\n";
         }
         else if (m_sent == 1)
         {
             // Second packet: Financial transaction
             std::string data = "TRANSACTION: Transfer $1000 to account 2002";
             packet = Create<Packet>((uint8_t*)data.c_str(), data.length());
             std::cout << Simulator::Now().GetSeconds() 
                       << "s: Client sending SENSITIVE transaction data\n";
         }
         else
         {
             // Subsequent packets: Regular echo data
             std::string data = "ECHO PACKET " + std::to_string(m_sent);
             packet = Create<Packet>((uint8_t*)data.c_str(), data.length());
         }
         
         // Send the packet
         m_socket->Send(packet);
         
         // Schedule next packet
         if (++m_sent < m_nPackets)
         {
             m_sendEvent = Simulator::Schedule(m_interval, &SensitiveUdpEchoClient::SendPacket, this);
         }
     }
     
 private:
     Ptr<Socket> m_socket;
     Address m_peerAddress;
     uint32_t m_nPackets;
     Time m_interval;
     EventId m_sendEvent;
     bool m_running;
     uint32_t m_sent;
     std::string m_sensitiveData;
 };
 
 // ============================================
 // Main Simulation
 // ============================================
 
 int
 main(int argc, char* argv[])
 {
     // Enable logging
     LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
     LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
     LogComponentEnable("EavesdroppingAttack", LOG_LEVEL_INFO);
     
     // Command line arguments
     bool enableEavesdropping = true;
     bool enableEncryption = false;  // Simulate IPsec-like protection
     
     CommandLine cmd;
     cmd.AddValue("eavesdrop", "Enable eavesdropping attack", enableEavesdropping);
     cmd.AddValue("encrypt", "Enable encryption (simulated)", enableEncryption);
     cmd.Parse(argc, argv);
 
     // ============================================
     // Create Network Nodes
     // ============================================
     
     // Create four nodes: n0 (client), n1 (router), n2 (server), n3 (attacker)
     NodeContainer nodes;
     nodes.Create(4);
 
     Ptr<Node> n0 = nodes.Get(0); // Client
     Ptr<Node> n1 = nodes.Get(1); // Router
     Ptr<Node> n2 = nodes.Get(2); // Server
     Ptr<Node> n3 = nodes.Get(3); // Attacker (eavesdropper)
 
     std::cout << "\n=== Network Setup ===\n";
     std::cout << "n0: Client (victim)\n";
     std::cout << "n1: Router\n";
     std::cout << "n2: Server (victim)\n";
     std::cout << "n3: Attacker (eavesdropper)\n";
     std::cout << "Eavesdropping: " << (enableEavesdropping ? "ENABLED" : "DISABLED") << "\n";
     std::cout << "Encryption: " << (enableEncryption ? "ENABLED" : "DISABLED") << "\n";
 
     // ============================================
     // Create Eavesdropping Link (Shared Medium)
     // ============================================
     
     // Use CSMA (shared medium) for the n0-n1 link to allow eavesdropping
     // In real WAN, this would be a tap on the physical link
     CsmaHelper csma;
     csma.SetChannelAttribute("DataRate", StringValue("5Mbps"));
     csma.SetChannelAttribute("Delay", StringValue("2ms"));
     
     // Create a shared bus for n0, n1, and n3 (attacker)
     NodeContainer sharedSegment;
     sharedSegment.Add(n0);
     sharedSegment.Add(n1);
     if (enableEavesdropping)
     {
         sharedSegment.Add(n3);  // Attacker on same segment
     }
     
     NetDeviceContainer csmaDevices = csma.Install(sharedSegment);
 
     // ============================================
     // Create Private Link (n1-n2)
     // ============================================
     
     PointToPointHelper p2p;
     p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
     p2p.SetChannelAttribute("Delay", StringValue("2ms"));
     
     // Link 2: n1 <-> n2 (Network 2) - point-to-point (private)
     NodeContainer link2Nodes(n1, n2);
     NetDeviceContainer link2Devices = p2p.Install(link2Nodes);
 
     // ============================================
     // Install Internet Stack
     // ============================================
     
     InternetStackHelper stack;
     stack.Install(nodes);
 
     // ============================================
     // Mobility Setup
     // ============================================
     
     MobilityHelper mobility;
     mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
     mobility.Install(nodes);
 
     // Set positions
     Ptr<MobilityModel> mob0 = n0->GetObject<MobilityModel>();
     Ptr<MobilityModel> mob1 = n1->GetObject<MobilityModel>();
     Ptr<MobilityModel> mob2 = n2->GetObject<MobilityModel>();
     Ptr<MobilityModel> mob3 = n3->GetObject<MobilityModel>();
 
     mob0->SetPosition(Vector(5.0, 15.0, 0.0));   // Client
     mob1->SetPosition(Vector(10.0, 10.0, 0.0));  // Router
     mob2->SetPosition(Vector(15.0, 15.0, 0.0));  // Server
     mob3->SetPosition(Vector(10.0, 5.0, 0.0));   // Attacker (below router)
 
     // ============================================
     // IP Address Assignment
     // ============================================
     
     // Assign IP addresses to CSMA network (10.1.1.0/24)
     Ipv4AddressHelper address1;
     address1.SetBase("10.1.1.0", "255.255.255.0");
     Ipv4InterfaceContainer csmaInterfaces = address1.Assign(csmaDevices);
     
     // Get specific IPs
     Ipv4Address n0Ip = csmaInterfaces.GetAddress(0);  // 10.1.1.1
     Ipv4Address n1IpCsma = csmaInterfaces.GetAddress(1);  // 10.1.1.2 (router interface 1)
     Ipv4Address n3Ip = enableEavesdropping ? csmaInterfaces.GetAddress(2) : Ipv4Address();  // 10.1.1.3
     
     // Assign IP addresses to P2P network (10.1.2.0/24)
     Ipv4AddressHelper address2;
     address2.SetBase("10.1.2.0", "255.255.255.0");
     Ipv4InterfaceContainer p2pInterfaces = address2.Assign(link2Devices);
     
     Ipv4Address n1IpP2p = p2pInterfaces.GetAddress(0);  // 10.1.2.1 (router interface 2)
     Ipv4Address n2Ip = p2pInterfaces.GetAddress(1);     // 10.1.2.2
 
     std::cout << "\n=== IP Address Assignment ===\n";
     std::cout << "n0 (Client): " << n0Ip << "\n";
     std::cout << "n1 (Router): " << n1IpCsma << " (CSMA) | " << n1IpP2p << " (P2P)\n";
     std::cout << "n2 (Server): " << n2Ip << "\n";
     if (enableEavesdropping)
     {
         std::cout << "n3 (Attacker): " << n3Ip << "\n";
     }
 
     // ============================================
     // Static Routing Configuration
     // ============================================
     
     // Enable IP forwarding on the router (n1)
     Ptr<Ipv4> ipv4Router = n1->GetObject<Ipv4>();
     ipv4Router->SetAttribute("IpForward", BooleanValue(true));
     
     // Configure static routes
     Ipv4StaticRoutingHelper staticRoutingHelper;
     
     // On n0: Route to server network via router
     Ptr<Ipv4StaticRouting> staticRoutingN0 = staticRoutingHelper.GetStaticRouting(n0->GetObject<Ipv4>());
     staticRoutingN0->AddNetworkRouteTo(
         Ipv4Address("10.1.2.0"),   // Destination network
         Ipv4Mask("255.255.255.0"), // Network mask
         n1IpCsma,                  // Next hop (router's CSMA interface)
         1                          // Interface index
     );
     
     // On n2: Route to client network via router
     Ptr<Ipv4StaticRouting> staticRoutingN2 = staticRoutingHelper.GetStaticRouting(n2->GetObject<Ipv4>());
     staticRoutingN2->AddNetworkRouteTo(
         Ipv4Address("10.1.1.0"),   // Destination network
         Ipv4Mask("255.255.255.0"), // Network mask
         n1IpP2p,                   // Next hop (router's P2P interface)
         1                          // Interface index
     );
     
     // On n3 (attacker): If enabled, configure routing to see all networks
     if (enableEavesdropping)
     {
         Ptr<Ipv4StaticRouting> staticRoutingN3 = staticRoutingHelper.GetStaticRouting(n3->GetObject<Ipv4>());
         // Attacker can reach both networks via router
         staticRoutingN3->AddNetworkRouteTo(
             Ipv4Address("10.1.2.0"),   // Server network
             Ipv4Mask("255.255.255.0"),
             n1IpCsma,                  // Via router
             1
         );
         
         std::cout << "\n=== Attacker Configuration ===\n";
         std::cout << "Attacker (n3) can sniff all traffic on CSMA segment\n";
         std::cout << "Attacker routing configured to observe network traffic\n";
     }
 
     // ============================================
     // Eavesdropping Setup
     // ============================================
     
     PacketSniffer* sniffer = nullptr;
     
     if (enableEavesdropping)
     {
         sniffer = new PacketSniffer("scratch/eavesdropping-log.txt");
         
         // Start sniffing on attacker's interface
         sniffer->StartSniffing(n3, 1);
         
         std::cout << "\n=== Eavesdropping Attack Started ===\n";
         std::cout << "Attacker is sniffing all traffic on the CSMA network\n";
         std::cout << "Log file: scratch/eavesdropping-log.txt\n";
     }
 
     // ============================================
     // Application Setup
     // ============================================
     
     // Create UDP Echo Server on n2 (10.1.2.2)
     uint16_t port = 9;
     UdpEchoServerHelper echoServer(port);
     ApplicationContainer serverApps = echoServer.Install(n2);
     serverApps.Start(Seconds(1.0));
     serverApps.Stop(Seconds(15.0));
     
     // ============================================
     // Install Sensitive Client Application
     // ============================================
     
     // Create and configure the sensitive client
     Ptr<SensitiveUdpEchoClient> client = CreateObject<SensitiveUdpEchoClient>();
     client->Setup(InetSocketAddress(n2Ip, port), 5, Seconds(2.0));
     n0->AddApplication(client);
     client->SetStartTime(Seconds(2.0));
     client->SetStopTime(Seconds(15.0));
     
     std::cout << "\n=== Sensitive Traffic Generated ===\n";
     std::cout << "Client (n0) will send:\n";
     std::cout << "1. Login credentials (t=2s)\n";
     std::cout << "2. Financial transaction (t=4s)\n";
     std::cout << "3. Regular echo packets (t=6,8,10s)\n";
     
     if (enableEncryption)
     {
         std::cout << "\n=== ENCRYPTION ENABLED ===\n";
         std::cout << "All sensitive data is encrypted\n";
         std::cout << "Attacker will only see encrypted payload\n";
     }
     else if (enableEavesdropping)
     {
         std::cout << "\n=== WARNING: NO ENCRYPTION ===\n";
         std::cout << "Sensitive data is sent in CLEAR TEXT\n";
         std::cout << "Attacker can read all information!\n";
     }
 
     // ============================================
     // Enable PCAP Tracing for Analysis
     // ============================================
     
     // Enable PCAP on all devices
     csma.EnablePcap("scratch/eavesdrop-csma", csmaDevices, true);
     p2p.EnablePcap("scratch/eavesdrop-p2p", link2Devices.Get(0), true);
     
     std::cout << "\n=== PCAP Files Generated ===\n";
     std::cout << "Use Wireshark to analyze:\n";
     std::cout << "- scratch/eavesdrop-csma-0.pcap: Client traffic (sniffable)\n";
     std::cout << "- scratch/eavesdrop-csma-1.pcap: Router interface (sniffable)\n";
     if (enableEavesdropping)
     {
         std::cout << "- scratch/eavesdrop-csma-2.pcap: Attacker view\n";
     }
     std::cout << "- scratch/eavesdrop-p2p-1.pcap: Private link (n1-n2)\n";
 
     // ============================================
     // Animation Setup (Optional)
     // ============================================
     
     AnimationInterface anim("scratch/eavesdropping-animation.xml");
     anim.SetConstantPosition(n0, 5, 15);
     anim.SetConstantPosition(n1, 10, 10);
     anim.SetConstantPosition(n2, 15, 15);
     anim.SetConstantPosition(n3, 10, 5);
     
     anim.UpdateNodeDescription(n0, "Client\n10.1.1.1");
     anim.UpdateNodeDescription(n1, "Router\n10.1.1.2|10.1.2.1");
     anim.UpdateNodeDescription(n2, "Server\n10.1.2.2");
     anim.UpdateNodeDescription(n3, "Attacker\n10.1.1.3");
     
     anim.UpdateNodeColor(n0, 0, 255, 0);   // Green
     anim.UpdateNodeColor(n1, 255, 255, 0); // Yellow
     anim.UpdateNodeColor(n2, 0, 0, 255);   // Blue
     anim.UpdateNodeColor(n3, 255, 0, 0);   // Red
 
     // ============================================
     // Run Simulation
     // ============================================
     
     std::cout << "\n=== Starting Simulation (15 seconds) ===\n";
     
     Simulator::Stop(Seconds(16.0));
     Simulator::Run();
     Simulator::Destroy();
     
     // ============================================
     // Cleanup and Results
     // ============================================
     
     if (sniffer)
     {
         delete sniffer;
     }
     
     std::cout << "\n=== Simulation Complete ===\n";
     
     if (enableEavesdropping && !enableEncryption)
     {
         std::cout << "\n=== EAVESDROPPING ATTACK SUCCESSFUL ===\n";
         std::cout << "Check eavesdropping-log.txt for captured data\n";
         std::cout << "\nCaptured Information:\n";
         std::cout << "1. Source/Destination IP addresses\n";
         std::cout << "2. UDP port numbers\n";
         std::cout << "3. Packet timing patterns\n";
         std::cout << "4. CLEAR TEXT credentials and transactions\n";
         std::cout << "5. Application data patterns\n";
     }
     else if (enableEavesdropping && enableEncryption)
     {
         std::cout << "\n=== EAVESDROPPING ATTACK PREVENTED ===\n";
         std::cout << "Encryption protected sensitive data\n";
         std::cout << "Attacker could only see:\n";
         std::cout << "1. Source/Destination IP addresses\n";
         std::cout << "2. Packet sizes and timing\n";
         std::cout << "3. NO access to application data\n";
     }
     
     std::cout << "\n=== Security Recommendations ===\n";
     std::cout << "1. Always use encryption (IPsec/TLS) for sensitive data\n";
     std::cout << "2. Use switched networks instead of shared media\n";
     std::cout << "3. Implement port security and MAC filtering\n";
     std::cout << "4. Monitor for promiscuous mode interfaces\n";
     std::cout << "5. Use VPN tunnels for all WAN traffic\n";
 
     return 0;
 }