
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/netanim-module.h"  // Add NetAnim module

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Ex2Qos");

int main (int argc, char *argv[])
{
  bool enableQos = true;
  bool enablePcap = false;

  CommandLine cmd;
  cmd.AddValue ("enableQos", "Enable FqCoDel on bottleneck", enableQos);
  cmd.AddValue ("enablePcap", "Enable PCAP tracing", enablePcap);
  cmd.Parse (argc, argv);

  Time::SetResolution (Time::NS);

  NodeContainer nodes;
  nodes.Create (3);
  Ptr<Node> n0 = nodes.Get (0);
  Ptr<Node> n1 = nodes.Get (1);
  Ptr<Node> n2 = nodes.Get (2);

  InternetStackHelper internet;
  internet.Install (nodes);

  // Bottleneck n0-n1
  PointToPointHelper p2pAccess;
  p2pAccess.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  p2pAccess.SetChannelAttribute ("Delay", StringValue ("2ms"));

  // Core n1-n2
  PointToPointHelper p2pCore;
  p2pCore.SetDeviceAttribute ("DataRate", StringValue ("20Mbps"));
  p2pCore.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer d0d1 = p2pAccess.Install (n0, n1);
  NetDeviceContainer d1d2 = p2pCore.Install (n1, n2);

  if (enableQos)
    {
      TrafficControlHelper tch;
      tch.SetRootQueueDisc ("ns3::FqCoDelQueueDisc");
      tch.Install (d0d1);
    }

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i0i1 = address.Assign (d0d1);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i1i2 = address.Assign (d1d2);

  Ipv4Address serverAddress = i1i2.GetAddress (1);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // ---- VoIP-like UDP flow ----
  uint16_t voipPort = 5000;
  UdpServerHelper voipServer (voipPort);
  ApplicationContainer voipServerApps = voipServer.Install (n2);
  voipServerApps.Start (Seconds (0.5));
  voipServerApps.Stop (Seconds (10.0));

  UdpClientHelper voipClient (serverAddress, voipPort);
  voipClient.SetAttribute ("MaxPackets", UintegerValue (100000));
  voipClient.SetAttribute ("Interval", TimeValue (MilliSeconds (20)));
  voipClient.SetAttribute ("PacketSize", UintegerValue (160));
  ApplicationContainer voipClientApps = voipClient.Install (n0);
  voipClientApps.Start (Seconds (1.0));
  voipClientApps.Stop (Seconds (10.0));

  // ---- FTP-like UDP flow ----
  uint16_t ftpPort = 6000;
  UdpServerHelper ftpServer (ftpPort);
  ApplicationContainer ftpServerApps = ftpServer.Install (n2);
  ftpServerApps.Start (Seconds (0.5));
  ftpServerApps.Stop (Seconds (10.0));

  UdpClientHelper ftpClient (serverAddress, ftpPort);
  ftpClient.SetAttribute ("MaxPackets", UintegerValue (1000000));
  ftpClient.SetAttribute ("Interval", TimeValue (MilliSeconds (2)));
  ftpClient.SetAttribute ("PacketSize", UintegerValue (1400));
  ApplicationContainer ftpClientApps = ftpClient.Install (n0);
  ftpClientApps.Start (Seconds (1.5));
  ftpClientApps.Stop (Seconds (10.0));

  FlowMonitorHelper flowmonHelper;
  Ptr<FlowMonitor> monitor = flowmonHelper.InstallAll ();

  if (enablePcap)
    {
      p2pAccess.EnablePcapAll ("ex2-qos-bottleneck");
      p2pCore.EnablePcapAll ("ex2-qos-core");
    }

  // Create NetAnim XML file
  AnimationInterface anim("ex2-qos-animation.xml");
  
  // Optional: Set node positions for better visualization
  anim.SetConstantPosition(n0, 10.0, 10.0);
  anim.SetConstantPosition(n1, 20.0, 10.0);
  anim.SetConstantPosition(n2, 30.0, 10.0);

  Simulator::Stop (Seconds (11.0));
  Simulator::Run ();

  monitor->SerializeToXmlFile ("ex2-qos-flow.xml", true, true);

  Simulator::Destroy ();
  return 0;
}