/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"

// Network Topology
//
//   Wifi 10.1.3.0  n3(ISP)
//                  | 
//  *  *  *  *  *   | (10.1.4.0)
//  |  |  |  |  |   |                (10.1.1.0)
// n4 n5 n6 n7 n8   n0(Router/Ap) -------------- n1
//   (10.1.3.0)     | 
//                  | (10.1.2.0)
//                  |
//                  n2             

//n1 and n2 send to n3
//n4 n5 n6 n7 n8 send to n3
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ThirdScriptExample");
class MyApp : public Application //the class which sends TCP packets to the TCP packet sink
{
public:
  MyApp ();
  virtual ~MyApp ();

  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

MyApp::~MyApp ()
{
  m_socket = 0;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}


int 
main (int argc, char *argv[])
{
  bool verbose = true;
  // uint32_t nCsma = 4;
  uint32_t nWifi = 5; //we have 5 wifi nodes
  bool tracing = true;//to create pcaps
  double error_rate = 0.000001;
  int simulation_time = 5; //seconds

  CommandLine cmd (__FILE__);
  // cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);

  cmd.Parse (argc,argv);

  // The underlying restriction of 18 is due to the grid position
  // allocator's configuration; the grid layout will exceed the
  // bounding box if more than 18 nodes are provided.
  if (nWifi > 18)
    {
      std::cout << "nWifi should be 18 or less; otherwise grid layout exceeds the bounding box" << std::endl;
      return 1;
    }

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

  NodeContainer n0n1; //point to point wired ethernet connecting PC1 to the router
  n0n1.Create (2);
  PointToPointHelper pointToPoint1;
  pointToPoint1.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint1.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NetDeviceContainer p2pDevices1;
  p2pDevices1 = pointToPoint1.Install (n0n1);

  NodeContainer n0n2; //point to point wired ethernet connecting PC2 to the router
  n0n2.Add(n0n1.Get(0));
  n0n2.Create (1);
  PointToPointHelper pointToPoint2;
  pointToPoint2.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint2.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NetDeviceContainer p2pDevices2;
  p2pDevices2 = pointToPoint2.Install (n0n2);


  NodeContainer n0n3;  //point to point wired ethernet connecting ISP server to the router
  n0n3.Add(n0n1.Get(0));
  n0n3.Create (1);
  PointToPointHelper pointToPoint3;
  pointToPoint3.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint3.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NetDeviceContainer p2pDevices3;
  p2pDevices3 = pointToPoint3.Install (n0n3);


  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi); //create 5 wifi nodes
  NodeContainer wifiApNode = n0n1.Get(0);//the wifi access point node at the router itself
// configuring the WiFi channnel
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy;
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.Install (wifiStaNodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);
// installing the Internet stack to all the nodes to help TCP work
  InternetStackHelper stack;
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);
  stack.Install(n0n1.Get(1));
  stack.Install(n0n2.Get(1));
  stack.Install(n0n3.Get(1));

  Ipv4AddressHelper address; //assigning IP addresses to the 8 nodes

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces1;
  p2pInterfaces1 = address.Assign (p2pDevices1);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces2;
  p2pInterfaces2 = address.Assign (p2pDevices2);

  address.SetBase ("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces3;
  p2pInterfaces3 = address.Assign (p2pDevices3);


  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer staInterface;
  staInterface = address.Assign (staDevices);
  address.Assign (apDevices);

  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();//error model at the router
  em->SetAttribute ("ErrorRate", DoubleValue (error_rate));
  p2pDevices1.Get(0)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

//sink for n2
  uint16_t sinkPort1 = 8080;//the first sink port at ISP node is at port 8080
  Address sinkAddress1 (InetSocketAddress (p2pInterfaces3.GetAddress (1), sinkPort1));
  PacketSinkHelper packetSinkHelper1 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort1));
  ApplicationContainer sinkApps1 = packetSinkHelper1.Install (n0n3.Get(1));
  sinkApps1.Start (Seconds (0.));
  sinkApps1.Stop (Seconds (simulation_time));

//sink for n1
  uint16_t sinkPort2 = 8081;
  Address sinkAddress2 (InetSocketAddress (p2pInterfaces3.GetAddress (1), sinkPort2));
  PacketSinkHelper packetSinkHelper2 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort2));
  ApplicationContainer sinkApps2 = packetSinkHelper2.Install (n0n3.Get(1));
  sinkApps2.Start (Seconds (0.));
  sinkApps2.Stop (Seconds (simulation_time));

  //sink for n4
  uint16_t sinkPort3 = 8082;
  Address sinkAddress3 (InetSocketAddress (p2pInterfaces3.GetAddress (1), sinkPort3));
  PacketSinkHelper packetSinkHelper3 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort3));
  ApplicationContainer sinkApps3 = packetSinkHelper3.Install (n0n3.Get(1));
  sinkApps3.Start (Seconds (0.));
  sinkApps3.Stop (Seconds (simulation_time));

  //sink for n5
  uint16_t sinkPort4 = 8083;
  Address sinkAddress4 (InetSocketAddress (p2pInterfaces3.GetAddress (1), sinkPort4));
  PacketSinkHelper packetSinkHelper4 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort4));
  ApplicationContainer sinkApps4 = packetSinkHelper4.Install (n0n3.Get(1));
  sinkApps4.Start (Seconds (0.));
  sinkApps4.Stop (Seconds (simulation_time));
  //sink for n6
  uint16_t sinkPort5 = 8084;
  Address sinkAddress5 (InetSocketAddress (p2pInterfaces3.GetAddress (1), sinkPort5));
  PacketSinkHelper packetSinkHelper5 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort5));
  ApplicationContainer sinkApps5 = packetSinkHelper5.Install (n0n3.Get(1));
  sinkApps5.Start (Seconds (0.));
  sinkApps5.Stop (Seconds (simulation_time));

  //sink for n7
  uint16_t sinkPort6 = 8085;
  Address sinkAddress6 (InetSocketAddress (p2pInterfaces3.GetAddress (1), sinkPort6));
  PacketSinkHelper packetSinkHelper6 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort6));
  ApplicationContainer sinkApps6 = packetSinkHelper6.Install (n0n3.Get(1));
  sinkApps6.Start (Seconds (0.));
  sinkApps6.Stop (Seconds (simulation_time));

  //sink for n8
  uint16_t sinkPort7 = 8086;
  Address sinkAddress7 (InetSocketAddress (p2pInterfaces3.GetAddress (1), sinkPort7));
  PacketSinkHelper packetSinkHelper7 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort7));
  ApplicationContainer sinkApps7 = packetSinkHelper7.Install (n0n3.Get(1));
  sinkApps7.Start (Seconds (0.));
  sinkApps7.Stop (Seconds (simulation_time));

// app at n2
  Ptr<Socket> ns3TcpSocket1 = Socket::CreateSocket (n0n2.Get (1), TcpSocketFactory::GetTypeId ());
  Ptr<MyApp> app1 = CreateObject<MyApp> ();
  app1->Setup (ns3TcpSocket1, sinkAddress1, 1460, 1000000, DataRate ("100Mbps"));
  n0n2.Get (1)->AddApplication (app1);
  app1->SetStartTime (Seconds (1.));
  app1->SetStopTime (Seconds (simulation_time));

  //app at n1
  Ptr<Socket> ns3TcpSocket2 = Socket::CreateSocket (n0n1.Get (1), TcpSocketFactory::GetTypeId ());
  Ptr<MyApp> app2 = CreateObject<MyApp> ();
  app2->Setup (ns3TcpSocket2, sinkAddress2, 1460, 1000000, DataRate ("100Mbps"));
  n0n1.Get (1)->AddApplication (app2);
  app2->SetStartTime (Seconds (1.));
  app2->SetStopTime (Seconds (simulation_time));

  //app at n4
  Ptr<Socket> ns3TcpSocket3 = Socket::CreateSocket (wifiStaNodes.Get(0), TcpSocketFactory::GetTypeId ());
  Ptr<MyApp> app3 = CreateObject<MyApp> ();
  app3->Setup (ns3TcpSocket3, sinkAddress3, 1460, 1000000, DataRate ("100Mbps"));
  wifiStaNodes.Get(0)->AddApplication (app3);
  app3->SetStartTime (Seconds (1.));
  app3->SetStopTime (Seconds (simulation_time));

  //app at n5
  Ptr<Socket> ns3TcpSocket4 = Socket::CreateSocket (wifiStaNodes.Get(1), TcpSocketFactory::GetTypeId ());
  Ptr<MyApp> app4 = CreateObject<MyApp> ();
  app4->Setup (ns3TcpSocket4, sinkAddress4, 1460, 1000000, DataRate ("100Mbps"));
  wifiStaNodes.Get(1)->AddApplication (app4);
  app4->SetStartTime (Seconds (1.));
  app4->SetStopTime (Seconds (simulation_time));

  //app at n6
  Ptr<Socket> ns3TcpSocket5 = Socket::CreateSocket (wifiStaNodes.Get(2), TcpSocketFactory::GetTypeId ());
  Ptr<MyApp> app5 = CreateObject<MyApp> ();
  app5->Setup (ns3TcpSocket5, sinkAddress5, 1460, 1000000, DataRate ("100Mbps"));
  wifiStaNodes.Get(2)->AddApplication (app5);
  app5->SetStartTime (Seconds (1.));
  app5->SetStopTime (Seconds (simulation_time));

  //app at n7
  Ptr<Socket> ns3TcpSocket6 = Socket::CreateSocket (wifiStaNodes.Get(3), TcpSocketFactory::GetTypeId ());
  Ptr<MyApp> app6 = CreateObject<MyApp> ();
  app6->Setup (ns3TcpSocket6, sinkAddress6, 1460, 1000000, DataRate ("100Mbps"));
  wifiStaNodes.Get(3)->AddApplication (app6);
  app6->SetStartTime (Seconds (1.));
  app6->SetStopTime (Seconds (simulation_time));

  //app at n8
  Ptr<Socket> ns3TcpSocket7 = Socket::CreateSocket (wifiStaNodes.Get(4), TcpSocketFactory::GetTypeId ());
  Ptr<MyApp> app7 = CreateObject<MyApp> ();
  app7->Setup (ns3TcpSocket7, sinkAddress7, 1460, 1000000, DataRate ("100Mbps"));
  wifiStaNodes.Get(4)->AddApplication (app7);
  app7->SetStartTime (Seconds (1.));
  app7->SetStopTime (Seconds (simulation_time));
//populate routing tables
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop (Seconds (simulation_time));
// create pcap files
  if (tracing)
    {
      phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
      pointToPoint1.EnablePcapAll ("p2p");
      phy.EnablePcapAll ("Wifi");
    }
// running the final simulator
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
