/*
 * Exercise 6 – Inter-AS Routing Simulation (AS65001 <-> AS65002 with 2 IXPs)
 */

 #include "ns3/core-module.h"
 #include "ns3/network-module.h"
 #include "ns3/internet-module.h"
 #include "ns3/point-to-point-module.h"
 #include "ns3/applications-module.h"
 #include "ns3/flow-monitor-module.h"
 #include "ns3/ipv4-static-routing-helper.h"
 #include "ns3/netanim-module.h"
 
 using namespace ns3;
 
 NS_LOG_COMPONENT_DEFINE ("Ex6InterAs");
 
 int main (int argc, char *argv[])
 {
   bool enablePcap = false;
   bool failIxpa   = false;
 
   CommandLine cmd;
   cmd.AddValue ("enablePcap", "Enable PCAP tracing", enablePcap);
   cmd.AddValue ("failIxpa",   "Simulate IXP-A failure", failIxpa);
   cmd.Parse (argc, argv);
 
   Time::SetResolution (Time::NS);
 
   NodeContainer nodes;
   nodes.Create (8);
 
   Ptr<Node> cust1  = nodes.Get (0);
   Ptr<Node> core1  = nodes.Get (1);
   Ptr<Node> edge1A = nodes.Get (2);
   Ptr<Node> edge1B = nodes.Get (3);
 
   Ptr<Node> cust2  = nodes.Get (4);
   Ptr<Node> core2  = nodes.Get (5);
   Ptr<Node> edge2A = nodes.Get (6);
   Ptr<Node> edge2B = nodes.Get (7);
 
   InternetStackHelper internet;
   internet.Install (nodes);
 
   PointToPointHelper p2pAccess;
   p2pAccess.SetDeviceAttribute ("DataRate", StringValue ("50Mbps"));
   p2pAccess.SetChannelAttribute ("Delay", StringValue ("2ms"));
 
   PointToPointHelper p2pCore;
   p2pCore.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
   p2pCore.SetChannelAttribute ("Delay", StringValue ("5ms"));
 
   PointToPointHelper p2pIxpA;
   p2pIxpA.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
   p2pIxpA.SetChannelAttribute ("Delay", StringValue ("10ms"));
 
   PointToPointHelper p2pIxpB;
   p2pIxpB.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
   p2pIxpB.SetChannelAttribute ("Delay", StringValue ("20ms"));
 
   NetDeviceContainer dCust1Core1   = p2pAccess.Install (cust1, core1);
   NetDeviceContainer dCore1Edge1A  = p2pCore.Install (core1, edge1A);
   NetDeviceContainer dCore1Edge1B  = p2pCore.Install (core1, edge1B);
 
   NetDeviceContainer dCust2Core2   = p2pAccess.Install (cust2, core2);
   NetDeviceContainer dCore2Edge2A  = p2pCore.Install (core2, edge2A);
   NetDeviceContainer dCore2Edge2B  = p2pCore.Install (core2, edge2B);
 
   NetDeviceContainer dEdge1AEdge2A = p2pIxpA.Install (edge1A, edge2A);
   NetDeviceContainer dEdge1BEdge2B = p2pIxpB.Install (edge1B, edge2B);
 
   Ipv4AddressHelper address;
 
   address.SetBase ("10.1.1.0", "255.255.255.0");
   Ipv4InterfaceContainer iCust1Core1 = address.Assign (dCust1Core1);
 
   address.SetBase ("10.1.2.0", "255.255.255.0");
   Ipv4InterfaceContainer iCore1Edge1A = address.Assign (dCore1Edge1A);
 
   address.SetBase ("10.1.3.0", "255.255.255.0");
   Ipv4InterfaceContainer iCore1Edge1B = address.Assign (dCore1Edge1B);
 
   address.SetBase ("10.2.1.0", "255.255.255.0");
   Ipv4InterfaceContainer iCust2Core2 = address.Assign (dCust2Core2);
 
   address.SetBase ("10.2.2.0", "255.255.255.0");
   Ipv4InterfaceContainer iCore2Edge2A = address.Assign (dCore2Edge2A);
 
   address.SetBase ("10.2.3.0", "255.255.255.0");
   Ipv4InterfaceContainer iCore2Edge2B = address.Assign (dCore2Edge2B);
 
   address.SetBase ("192.0.2.0", "255.255.255.252");
   Ipv4InterfaceContainer iEdge1AEdge2A = address.Assign (dEdge1AEdge2A);
 
   address.SetBase ("198.51.100.0", "255.255.255.252");
   Ipv4InterfaceContainer iEdge1BEdge2B = address.Assign (dEdge1BEdge2B);
 
   Ipv4Address cust1Ip  = iCust1Core1.GetAddress (0);
   Ipv4Address core1Ip  = iCust1Core1.GetAddress (1);
   Ipv4Address edge1AIp = iCore1Edge1A.GetAddress (1);
   Ipv4Address edge1BIp = iCore1Edge1B.GetAddress (1);
 
   Ipv4Address cust2Ip  = iCust2Core2.GetAddress (0);
   Ipv4Address core2Ip  = iCust2Core2.GetAddress (1);
   Ipv4Address edge2AIp = iCore2Edge2A.GetAddress (1);
   Ipv4Address edge2BIp = iCore2Edge2B.GetAddress (1);
 
   Ipv4Address ixpA_1   = iEdge1AEdge2A.GetAddress (0);
   Ipv4Address ixpA_2   = iEdge1AEdge2A.GetAddress (1);
 
   Ipv4Address ixpB_1   = iEdge1BEdge2B.GetAddress (0);
   Ipv4Address ixpB_2   = iEdge1BEdge2B.GetAddress (1);
 
   Ipv4StaticRoutingHelper staticHelper;
 
   Ptr<Ipv4StaticRouting> rCust1  = staticHelper.GetStaticRouting (cust1->GetObject<Ipv4> ());
   Ptr<Ipv4StaticRouting> rCore1  = staticHelper.GetStaticRouting (core1->GetObject<Ipv4> ());
   Ptr<Ipv4StaticRouting> rEdge1A = staticHelper.GetStaticRouting (edge1A->GetObject<Ipv4> ());
   Ptr<Ipv4StaticRouting> rEdge1B = staticHelper.GetStaticRouting (edge1B->GetObject<Ipv4> ());
 
   Ptr<Ipv4StaticRouting> rCust2  = staticHelper.GetStaticRouting (cust2->GetObject<Ipv4> ());
   Ptr<Ipv4StaticRouting> rCore2  = staticHelper.GetStaticRouting (core2->GetObject<Ipv4> ());
   Ptr<Ipv4StaticRouting> rEdge2A = staticHelper.GetStaticRouting (edge2A->GetObject<Ipv4> ());
   Ptr<Ipv4StaticRouting> rEdge2B = staticHelper.GetStaticRouting (edge2B->GetObject<Ipv4> ());
 
   // Customer defaults
   rCust1->SetDefaultRoute (core1Ip, 1);
   rCust2->SetDefaultRoute (core2Ip, 1);
 
   // Core1: primary to 10.2.0.0/16 via edge1A, backup via edge1B
   rCore1->AddNetworkRouteTo (Ipv4Address ("10.2.0.0"), Ipv4Mask ("255.255.0.0"),
                              edge1AIp, 2);
   rCore1->AddNetworkRouteTo (Ipv4Address ("10.2.0.0"), Ipv4Mask ("255.255.0.0"),
                              edge1BIp, 3);
 
   // Edge1A
   rEdge1A->AddNetworkRouteTo (Ipv4Address ("10.2.0.0"), Ipv4Mask ("255.255.0.0"),
                               ixpA_2, 2);
   rEdge1A->AddNetworkRouteTo (Ipv4Address ("10.1.0.0"), Ipv4Mask ("255.255.0.0"),
                               core1Ip, 1);
 
   // Edge1B
   rEdge1B->AddNetworkRouteTo (Ipv4Address ("10.2.0.0"), Ipv4Mask ("255.255.0.0"),
                               ixpB_2, 2);
   rEdge1B->AddNetworkRouteTo (Ipv4Address ("10.1.0.0"), Ipv4Mask ("255.255.0.0"),
                               core1Ip, 1);
 
   // Core2
   rCore2->AddNetworkRouteTo (Ipv4Address ("10.1.0.0"), Ipv4Mask ("255.255.0.0"),
                              edge2AIp, 2);
   rCore2->AddNetworkRouteTo (Ipv4Address ("10.1.0.0"), Ipv4Mask ("255.255.0.0"),
                              edge2BIp, 3);
 
   // Edge2A
   rEdge2A->AddNetworkRouteTo (Ipv4Address ("10.1.0.0"), Ipv4Mask ("255.255.0.0"),
                               ixpA_1, 2);
   rEdge2A->AddNetworkRouteTo (Ipv4Address ("10.2.0.0"), Ipv4Mask ("255.255.0.0"),
                               core2Ip, 1);
 
   // Edge2B
   rEdge2B->AddNetworkRouteTo (Ipv4Address ("10.1.0.0"), Ipv4Mask ("255.255.0.0"),
                               ixpB_1, 2);
   rEdge2B->AddNetworkRouteTo (Ipv4Address ("10.2.0.0"), Ipv4Mask ("255.255.0.0"),
                               core2Ip, 1);
 
   // UDP echo cust1 -> cust2
   uint16_t echoPort = 9;
   UdpEchoServerHelper echoServer (echoPort);
   ApplicationContainer serverApps = echoServer.Install (cust2);
   serverApps.Start (Seconds (1.0));
   serverApps.Stop (Seconds (20.0));
 
   UdpEchoClientHelper echoClient (cust2Ip, echoPort);
   echoClient.SetAttribute ("MaxPackets", UintegerValue (100000));
   echoClient.SetAttribute ("Interval", TimeValue (Seconds (0.5)));
   echoClient.SetAttribute ("PacketSize", UintegerValue (128));
   ApplicationContainer clientApps = echoClient.Install (cust1);
   clientApps.Start (Seconds (2.0));
   clientApps.Stop (Seconds (20.0));
 
   if (failIxpa)
     {
       Ptr<NetDevice> ixpaDevice1 = dEdge1AEdge2A.Get (0);
       Ptr<NetDevice> ixpaDevice2 = dEdge1AEdge2A.Get (1);
       
       Simulator::Schedule (Seconds (8.0), [ixpaDevice1]() {
         ixpaDevice1->SetAttribute ("LinkDown", BooleanValue (true));
       });
       
       Simulator::Schedule (Seconds (8.0), [ixpaDevice2]() {
         ixpaDevice2->SetAttribute ("LinkDown", BooleanValue (true));
       });
     }
 
   FlowMonitorHelper flowHelper;
   Ptr<FlowMonitor> monitor = flowHelper.InstallAll ();
 
   if (enablePcap)
     {
       p2pAccess.EnablePcapAll ("ex6-interas-access");
       p2pCore.EnablePcapAll   ("ex6-interas-core");
       p2pIxpA.EnablePcapAll   ("ex6-interas-ixpa");
       p2pIxpB.EnablePcapAll   ("ex6-interas-ixpb");
     }
 
   // Create NetAnim XML file
   AnimationInterface anim("ex6-interas-animation.xml");
   
   // Set node positions for better visualization (two ASes side by side)
   // AS65001 nodes on the left
   anim.SetConstantPosition(cust1,  10.0, 20.0);
   anim.SetConstantPosition(core1,  30.0, 20.0);
   anim.SetConstantPosition(edge1A, 50.0, 30.0);
   anim.SetConstantPosition(edge1B, 50.0, 10.0);
   
   // AS65002 nodes on the right
   anim.SetConstantPosition(cust2,  90.0, 20.0);
   anim.SetConstantPosition(core2,  70.0, 20.0);
   anim.SetConstantPosition(edge2A, 60.0, 30.0);
   anim.SetConstantPosition(edge2B, 60.0, 10.0);
 
   Simulator::Stop (Seconds (21.0));
   Simulator::Run ();
 
   monitor->SerializeToXmlFile ("ex6-interas-flow.xml", true, true);
 
   Simulator::Destroy ();
   return 0;
 }