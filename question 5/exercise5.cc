/*
 * Exercise 5 – Policy-Based Routing (simulated by two flows, two paths)
 */

 #include "ns3/core-module.h"
 #include "ns3/network-module.h"
 #include "ns3/internet-module.h"
 #include "ns3/point-to-point-module.h"
 #include "ns3/applications-module.h"
 #include "ns3/flow-monitor-module.h"
 #include "ns3/netanim-module.h"
 #include "ns3/mobility-module.h"
 
 using namespace ns3;
 
 NS_LOG_COMPONENT_DEFINE ("Ex5Pbr");
 
 int main (int argc, char *argv[])
 {
   bool enablePcap = false;
   CommandLine cmd;
   cmd.AddValue ("enablePcap", "Enable PCAP tracing", enablePcap);
   cmd.Parse (argc, argv);
 
   Time::SetResolution (Time::NS);
 
   NodeContainer nodes;
   nodes.Create (4); // 0=Studio, 1=Router, 2=CloudA, 3=CloudB
 
   Ptr<Node> studio = nodes.Get (0);
   Ptr<Node> router = nodes.Get (1);
   Ptr<Node> cloudA = nodes.Get (2);
   Ptr<Node> cloudB = nodes.Get (3);
 
   // Create mobility models for all nodes
   MobilityHelper mobility;
   
   // Set positions for nodes (stationary positions)
   Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
   positionAlloc->Add(Vector(0.0, 0.0, 0.0));      // Studio at (0,0)
   positionAlloc->Add(Vector(50.0, 0.0, 0.0));     // Router at (50,0)
   positionAlloc->Add(Vector(100.0, 30.0, 0.0));   // CloudA at (100,30)
   positionAlloc->Add(Vector(100.0, -30.0, 0.0));  // CloudB at (100,-30)
   
   mobility.SetPositionAllocator(positionAlloc);
   mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
   mobility.Install(nodes);
 
   InternetStackHelper internet;
   internet.Install (nodes);
 
   PointToPointHelper p2pLan;
   p2pLan.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
   p2pLan.SetChannelAttribute ("Delay", StringValue ("1ms"));
 
   PointToPointHelper p2pPathA; // low-latency
   p2pPathA.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
   p2pPathA.SetChannelAttribute ("Delay", StringValue ("5ms"));
 
   PointToPointHelper p2pPathB; // higher latency
   p2pPathB.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
   p2pPathB.SetChannelAttribute ("Delay", StringValue ("20ms"));
 
   NetDeviceContainer d0d1 = p2pLan.Install (studio, router);
   NetDeviceContainer d1d2 = p2pPathA.Install (router, cloudA);
   NetDeviceContainer d1d3 = p2pPathB.Install (router, cloudB);
 
   Ipv4AddressHelper address;
 
   address.SetBase ("10.20.1.0", "255.255.255.0");
   Ipv4InterfaceContainer i0i1 = address.Assign (d0d1);
 
   address.SetBase ("10.20.2.0", "255.255.255.0");
   Ipv4InterfaceContainer i1i2 = address.Assign (d1d2);
 
   address.SetBase ("10.20.3.0", "255.255.255.0");
   Ipv4InterfaceContainer i1i3 = address.Assign (d1d3);
 
   Ipv4Address cloudAIp   = i1i2.GetAddress (1);
   Ipv4Address cloudBIp   = i1i3.GetAddress (1);
 
   Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
 
   // Flow_Video -> CloudA
   uint16_t videoPort = 5000;
   UdpServerHelper videoServer (videoPort);
   ApplicationContainer videoServerApps = videoServer.Install (cloudA);
   videoServerApps.Start (Seconds (0.5));
   videoServerApps.Stop (Seconds (20.0));
 
   UdpClientHelper videoClient (cloudAIp, videoPort);
   videoClient.SetAttribute ("MaxPackets", UintegerValue (100000));
   videoClient.SetAttribute ("Interval", TimeValue (MilliSeconds (10)));
   videoClient.SetAttribute ("PacketSize", UintegerValue (500));
   ApplicationContainer videoClientApps = videoClient.Install (studio);
   videoClientApps.Start (Seconds (1.0));
   videoClientApps.Stop (Seconds (20.0));
 
   // Flow_Data -> CloudB
   uint16_t dataPort = 6000;
   UdpServerHelper dataServer (dataPort);
   ApplicationContainer dataServerApps = dataServer.Install (cloudB);
   dataServerApps.Start (Seconds (0.5));
   dataServerApps.Stop (Seconds (20.0));
 
   UdpClientHelper dataClient (cloudBIp, dataPort);
   dataClient.SetAttribute ("MaxPackets", UintegerValue (100000));
   dataClient.SetAttribute ("Interval", TimeValue (MilliSeconds (50)));
   dataClient.SetAttribute ("PacketSize", UintegerValue (1200));
   ApplicationContainer dataClientApps = dataClient.Install (studio);
   dataClientApps.Start (Seconds (1.5));
   dataClientApps.Stop (Seconds (20.0));
 
   // Create Animation Interface for NetAnim
   AnimationInterface anim("ex5-pbr-animation.xml");
   
   // Update node descriptions for better visualization
   anim.UpdateNodeDescription(studio, "Studio");
   anim.UpdateNodeDescription(router, "Router");
   anim.UpdateNodeDescription(cloudA, "CloudA");
   anim.UpdateNodeDescription(cloudB, "CloudB");
   
   // Update node color
   anim.UpdateNodeColor(studio, 0, 255, 0);    // Green
   anim.UpdateNodeColor(router, 255, 255, 0);  // Yellow
   anim.UpdateNodeColor(cloudA, 0, 0, 255);    // Blue
   anim.UpdateNodeColor(cloudB, 255, 0, 0);    // Red
 
   // Track packets for visualization (optional)
   anim.EnablePacketMetadata(true);
   anim.EnableIpv4L3ProtocolCounters(Seconds(0), Seconds(21));
 
   FlowMonitorHelper flowmonHelper;
   Ptr<FlowMonitor> monitor = flowmonHelper.InstallAll ();
 
   if (enablePcap)
     {
       p2pLan.EnablePcapAll ("ex5-pbr-lan");
       p2pPathA.EnablePcapAll ("ex5-pbr-pathA");
       p2pPathB.EnablePcapAll ("ex5-pbr-pathB");
     }
 
   Simulator::Stop (Seconds (21.0));
   Simulator::Run ();
 
   monitor->SerializeToXmlFile ("ex5-pbr-flow.xml", true, true);
 
   Simulator::Destroy ();
   return 0;
 }