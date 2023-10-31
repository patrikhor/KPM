/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011, 2013 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
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
 *
 * Author: Pavel Masek <masekpavel@vutbr.cz>
 *         Dinh Thao Le <Dinh.Thao.Le@vutbr.cz>
*/

#include <fstream>
#include <string>

#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store-module.h"

#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/data-rate.h"
#include "ns3/gnuplot.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("lte-full");

int main(int argc, char *argv[]) {

  uint16_t numberOfUes = 10;
  uint16_t numberOf_eNodeBs = 2;
  double simTime = 10.0;
  double interval = 50.0; // ms
  double distance = 200.0;
  bool useCa = true;
  Time interPacketInterval = MilliSeconds (200);

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue("numberOfUes", "Number of UEs", numberOfUes);
  cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue("distance", "Distance between eNBs [m]", distance);
  cmd.AddValue("useCa", "Whether to use carrier aggregation.", useCa);
  cmd.AddValue("interval", "Inter-packet interval for UDP client [ms]", interval);
  cmd.AddValue ("interPacketInterval", "Inter packet interval", interPacketInterval);
  cmd.Parse(argc, argv);

  if (useCa) {
      Config::SetDefault("ns3::LteHelper::UseCa", BooleanValue(useCa));
      Config::SetDefault("ns3::LteHelper::NumberOfComponentCarriers", UintegerValue(2));
      Config::SetDefault("ns3::LteHelper::EnbComponentCarrierManager", StringValue("ns3::RrComponentCarrierManager"));
  }

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> (); // create LteHelper object
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> (); // PointToPointEpcHelper
  lteHelper->SetEpcHelper (epcHelper); // enable the use of EPC by LTE helper

  lteHelper->SetEnbDeviceAttribute("DlBandwidth", UintegerValue(100));
  lteHelper->SetEnbDeviceAttribute("UlBandwidth", UintegerValue(100));

  Ptr<Node> pgw = epcHelper->GetPgwNode (); // get the PGW node

  // Create a single RemoteHost
  NodeContainer remoteHostContainer; // container for remote node
  remoteHostContainer.Create (1); // create 1 node
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer); // aggregate stack implementations (ipv4, ipv6, udp, tcp) to remote node

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s"))); // p2p data rate
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500)); // p2p mtu
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010))); // p2p delay
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0"); // allocates IP addresses (network number and  mask), p2p interface
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices); // assign IPs to each p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create(numberOf_eNodeBs); // create eNB nodes
  ueNodes.Create(numberOfUes); // create UE nodes

  // Install Mobility Model
  Ptr<ListPositionAllocator> positionAllocEnb = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < numberOf_eNodeBs; i++)
    {
      positionAllocEnb->Add (Vector (200 + distance * i, 300, 0));
      
    }
  
  
  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel"); 
  mobility.SetPositionAllocator(positionAllocEnb);
  mobility.Install(enbNodes);

  mobility.SetPositionAllocator("ns3::RandomBoxPositionAllocator",
                                "X", StringValue("ns3::UniformRandomVariable[Min=100|Max=500]"),
                                "Y", StringValue("ns3::UniformRandomVariable[Min=200|Max=400]"),
                                "Z", StringValue("ns3::UniformRandomVariable[Min=0|Max=0]"));
  mobility.Install(ueNodes);
  

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes); // add eNB nodes to the container
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes); // add UE nodes to the container

  Ptr<NetDevice> enbNetDev = enbLteDevs.Get(0);
  Ptr<LteEnbNetDevice> enbLteNetDev = DynamicCast<LteEnbNetDevice>(enbNetDev);
  //enbLteNetDev->SetDlBandwidth(100);
  //enbLteNetDev->SetUlBandwidth(100);
  uint16_t UL_bandwith = enbLteNetDev->GetUlBandwidth();
  uint16_t DL_bandwith= enbLteNetDev->GetDlBandwidth();
  uint32_t UlEarfcn = enbLteNetDev->GetUlEarfcn();
  uint32_t DlEarfcn = enbLteNetDev->GetDlEarfcn();
  Ptr<LteEnbPhy> enbPhy = enbLteNetDev->GetPhy();

  double txPower = enbPhy->GetTxPower();

  std::cout << "DL Earfcn: " << DlEarfcn <<std::endl
            << "UL Earfcn: " << UlEarfcn <<std::endl
            << "DL BW: " << DL_bandwith <<std::endl
            << "UL BW: " << UL_bandwith <<std::endl
            << "enb TxPower: " << txPower <<std::endl;
  
  Ptr<NetDevice> ueNetDev = ueLteDevs.Get(0);
  Ptr<LteUeNetDevice> ueLteNetDev = DynamicCast<LteUeNetDevice>(ueNetDev);
  Ptr<LteUePhy> uePhy = ueLteNetDev->GetPhy();

  double uetxPower = uePhy->GetTxPower();
  std::cout << "Ue TxPower: " << uetxPower <<std::endl;


  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  // Assign IP address to UEs
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1); // default route
    }

  // Attach UEs to eNodeBs
  lteHelper->Attach(ueLteDevs);

  // Create BulkSendApplication as 1st client application
  uint16_t port = 9; // First half of UEs
  BulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(remoteHostAddr, port));
  // Set the amount of data to send in bytes. Zero is unlimited .
  source.SetAttribute("MaxBytes", UintegerValue(200000)); //200 kB
  source.SetAttribute("SendSize", UintegerValue(100));
  uint16_t dlPort = 33;
  UdpClientHelper ulClient (remoteHostAddr, dlPort); // udp client
  ulClient.SetAttribute ("Interval", TimeValue (interPacketInterval));
  ulClient.SetAttribute ("MaxPackets", UintegerValue (1000000));


  ApplicationContainer sourceApps;
  ApplicationContainer sourceApps2;



  for (uint16_t u = 0; u < numberOfUes; u++)
    {
          sourceApps.Add(source.Install(ueNodes.Get(u)));
          u++;
          sourceApps2.Add(ulClient.Install(ueNodes.Get(u)));

    }

  sourceApps.Start(Seconds(0.5));
  sourceApps2.Start(Seconds(0.5));
  

  // Create a PacketSinkApplication and install it on Remote host TCP
  PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
  ApplicationContainer sinkApps = sink.Install(remoteHost); // sink apps
  sinkApps.Start(Seconds(0.5));

  // Create a PacketSinkApplication and install it on Remote host UDP
  PacketSinkHelper sink2("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort));
  ApplicationContainer sinkApps2 = sink2.Install(remoteHost); // sink apps
  sinkApps2.Start(Seconds(0.5));

  // Uncomment to enable traces
  // lteHelper->EnableTraces();

  // Animation definition
  AnimationInterface anim("lte-full.xml");
  

  /// Optional step
  anim.SetMobilityPollInterval (Seconds (0.75));
  
  // Uncomment to enable recording of packet Metadata
  // anim.EnablePacketMetadata(true);

  unsigned long long maxAnimPackets = 0xFFFFFFFFFFFFFFFF;
  anim.SetMaxPktsPerTraceFile(maxAnimPackets);
  
  anim.UpdateNodeDescription(pgw, "PGW");
  anim.UpdateNodeDescription(remoteHost, "RemoteHost");
  anim.UpdateNodeDescription(1, "SGW");
  anim.UpdateNodeDescription(2, "MME");

  for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
      anim.UpdateNodeDescription(ueNodes.Get(u), "Ue_" + std::to_string(u));
      anim.UpdateNodeColor(ueNodes.Get(u), 0, 0, 255); // Optional
   }

  for (uint32_t u = 0; u < enbNodes.GetN(); ++u)
    {
      anim.UpdateNodeDescription(enbNodes.Get(u), "eNodeB_" + std::to_string(u));
      anim.UpdateNodeColor(enbNodes.Get(u), 0, 255, 0); // Optional
    }

  // Uncomment to enable PCAP tracing
  p2ph.EnablePcapAll("lte-full");

  Ptr <FlowMonitor> monitor; // = flowMonHelper.InstallAll();
  FlowMonitorHelper flowMonHelper;
  monitor = flowMonHelper.Install(enbNodes);
  monitor = flowMonHelper.Install(ueNodes);
  monitor = flowMonHelper.Install(remoteHost);

  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  // GnuPlot
  std::string jmenoSouboru = "delay";
  std::string graphicsFileName = jmenoSouboru + ".png";
  std::string plotFileName = jmenoSouboru + ".plt";
  std::string plotTitle = "Average delay";
  std::string dataTitle = "Delay [ms]";
  Gnuplot gnuplot(graphicsFileName);
  gnuplot.SetTitle(plotTitle);
  gnuplot.SetTerminal("png");
  gnuplot.SetLegend("IDs of data streams", "Delay [ms]");
  gnuplot.AppendExtra("set xrange [1:" + std::to_string(numberOfUes * 2) + "]");
  gnuplot.AppendExtra("set yrange [0:500]");
  gnuplot.AppendExtra("set grid");
  Gnuplot2dDataset dataset_delay;

  jmenoSouboru = "datarate";
  graphicsFileName = jmenoSouboru + ".png";
  std::string plotFileNameDR = jmenoSouboru + ".plt";
  plotTitle = "Data rate for IDs";
  dataTitle = "Data rate [kbps]";
  Gnuplot gnuplot_DR(graphicsFileName);
  gnuplot_DR.SetTitle(plotTitle);
  gnuplot_DR.SetTerminal("png");
  gnuplot_DR.SetLegend("IDs of all streams", "Data rate [kbps]");
  gnuplot_DR.AppendExtra("set xrange [1:" + std::to_string(numberOfUes * 2) + "]");
  gnuplot_DR.AppendExtra("set yrange [0:2500]");
  gnuplot_DR.AppendExtra("set grid");
  Gnuplot2dDataset dataset_rate;

  monitor->CheckForLostPackets();
  Ptr <Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowMonHelper.GetClassifier());
  std::map <FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

  monitor->SerializeToXmlFile("lte-full.flowmon", true, true);

  std::cout << std::endl << "*** Flow monitor statistic ***" << std::endl;
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i) {

      // if (i-> first > 2) {
      double Delay, DataRate;
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
      std::cout << "Flow ID: " << i->first << std::endl;
      std::cout << "Src add: " << t.sourceAddress << "-> Dst add: " << t.destinationAddress << std::endl;
      std::cout << "Src port: " << t.sourcePort << "-> Dst port: " << t.destinationPort << std::endl;
      std::cout << "Tx Packets/Bytes: " << i->second.txPackets << "/" << i->second.txBytes << std::endl;
      std::cout << "Rx Packets/Bytes: " << i->second.rxPackets << "/" << i->second.rxBytes << std::endl;
      std::cout << "Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds()) / 1024 << "kb/s" << std::endl;
      std::cout << "Delay sum: " << i->second.delaySum.GetMilliSeconds() << "ms" << std::endl;
      std::cout << "Mean delay: " << (i->second.delaySum.GetSeconds() / i->second.rxPackets) * 1000 << "ms" << std::endl;

      // gnuplot Delay
      Delay = (i->second.delaySum.GetSeconds() / i->second.rxPackets) * 1000;
      DataRate = i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds()) / 1024;

      dataset_delay.Add((double) i->first, (double) Delay);
      dataset_rate.Add((double) i->first, (double) DataRate);
      std::cout << "Jitter sum: " << i->second.jitterSum.GetMilliSeconds() << "ms" << std::endl;
      std::cout << "Mean jitter: " << (i->second.jitterSum.GetSeconds() / (i->second.rxPackets - 1)) * 1000 << "ms" << std::endl;
      // std::cout << "Lost Packets: " << i->second.lostPackets << std::endl;
      std::cout << "Lost Packets: " << i->second.txPackets - i->second.rxPackets << std::endl;
      std::cout << "Packet loss: " << (((i->second.txPackets - i->second.rxPackets) * 1.0) / i->second.txPackets) * 100 << "%" << std::endl;
      std::cout << "------------------------------------------------" << std::endl;

  }




  // Gnuplot - continuation
  gnuplot.AddDataset(dataset_delay);
  std::ofstream plotFile(plotFileName.c_str());
  gnuplot.GenerateOutput(plotFile);
  plotFile.close();
  gnuplot_DR.AddDataset(dataset_rate);
  std::ofstream plotFileDR(plotFileNameDR.c_str());
  gnuplot_DR.GenerateOutput(plotFileDR);
  plotFileDR.close();

  Simulator::Destroy();
  return 0;
}
