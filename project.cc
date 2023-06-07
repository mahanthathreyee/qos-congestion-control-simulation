#include "project-helper.h"

using namespace ns3;

#pragma region Configure Simulation Defaults
void configureDefaults() {
    Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                        TypeIdValue(TypeId::LookupByName(transportProtocol)));

    Config::SetDefault("ns3::TcpSocket::SegmentSize", 
                        UintegerValue(segmentSize));
}
#pragma endregion

int main(int argc, char* argv[]) {
    #pragma region Command Line Arguments
    CommandLine cmd(__FILE__);
    cmd.AddValue("transportProtocol", "Transport protocol to be used", transportProtocol);
    cmd.AddValue("flowStatFile", "File path to store flow stats", flowStatFile);
    cmd.Parse(argc, argv);
    #pragma endregion

    #pragma region Utility Helper 
    configureDefaults();
    
    // Create Nodes
    NodeContainer sourceNodes;
    NodeContainer routers;
    NodeContainer sinkNodes;
    sourceNodes.Create(4);
    routers.Create(2);
    sinkNodes.Create(2);

    // Install Internet Stack for IP Address allocation
    InternetStackHelper stack;
    stack.Install(sourceNodes);
    stack.Install(routers);
    stack.Install(sinkNodes);

    // Clear Stats File
    truncateFile();

    Ipv4AddressHelper ipv4;
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
    #pragma endregion

    #pragma region Configure nodes
    // Left Nodes
    NodeContainer l0c0 = NodeContainer(sourceNodes.Get(0), routers.Get(0));
    NodeContainer l1c0 = NodeContainer(sourceNodes.Get(1), routers.Get(0));
    NodeContainer l2c0 = NodeContainer(sourceNodes.Get(2), routers.Get(1));
    NodeContainer l3c0 = NodeContainer(sourceNodes.Get(3), routers.Get(1));
    // Right Nodes
    NodeContainer r0c1 = NodeContainer(routers.Get(1), sinkNodes.Get(0));
    NodeContainer r1c1 = NodeContainer(routers.Get(1), sinkNodes.Get(1));

    PointToPointHelper nodeRouterLink;
    NetDeviceContainer ld0cd0 = nodeRouterLink.Install(l0c0);
    NetDeviceContainer ld1cd0 = nodeRouterLink.Install(l1c0);
    NetDeviceContainer ld2cd0 = nodeRouterLink.Install(l2c0);
    NetDeviceContainer ld3cd0 = nodeRouterLink.Install(l3c0);
    NetDeviceContainer rd0cd1 = nodeRouterLink.Install(r0c1);
    NetDeviceContainer rd1cd1 = nodeRouterLink.Install(r1c1);

    // Configure IP Address
    // Left Side Nodes
    ipv4.SetBase("10.1.1.0","255.255.255.0");
    Ipv4InterfaceContainer regLinkInterface_ld0cd0 = ipv4.Assign(ld0cd0);
    ipv4.SetBase("10.1.2.0","255.255.255.0");
    Ipv4InterfaceContainer regLinkInterface_ld1cd0 = ipv4.Assign(ld1cd0);
    ipv4.SetBase("10.1.3.0","255.255.255.0");
    Ipv4InterfaceContainer regLinkInterface_ld2cd0 = ipv4.Assign(ld2cd0);
    ipv4.SetBase("10.1.4.0","255.255.255.0");
    Ipv4InterfaceContainer regLinkInterface_ld3cd0 = ipv4.Assign(ld3cd0);
    // Right Side Nodes
    ipv4.SetBase("10.2.1.0","255.255.255.0");
    Ipv4InterfaceContainer regLinkInterface_rd0cd1 = ipv4.Assign(rd0cd1);
    ipv4.SetBase("10.2.2.0","255.255.255.0");
    Ipv4InterfaceContainer regLinkInterface_rd1cd1 = ipv4.Assign(rd1cd1);
    #pragma endregion

    #pragma region Configure routers
    NodeContainer n2n3 = NodeContainer(routers.Get(0), routers.Get(1));
    PointToPointHelper bottleNeckLinkA;
    
    NetDeviceContainer cd0cd1 = bottleNeckLinkA.Install(n2n3);
    bottleNeckLinkA.SetDeviceAttribute ("DataRate", StringValue ("1Kbps"));

    // Configure IP Address
    ipv4.SetBase("10.3.1.0","255.255.255.0");
    Ipv4InterfaceContainer bottleneckInterface = ipv4.Assign(cd0cd1);
    #pragma endregion Configure routers

    #pragma region Utility Helper 
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    #pragma endregion

    #pragma region Configure Application 
    // Configure Application Sink On Right Nodes
    Address sinkAddress4(
        InetSocketAddress(regLinkInterface_rd0cd1.GetAddress(1), sinkPort));
    Address sinkAddress5(
        InetSocketAddress(regLinkInterface_rd1cd1.GetAddress(1), sinkPort));
    ApplicationContainer sinkApps4 = packetSinkHelper.Install(sinkNodes.Get(0));
    ApplicationContainer sinkApps5 = packetSinkHelper.Install(sinkNodes.Get(1));
    sinkApps4.Start(Seconds(0));
    sinkApps4.Stop(simulationEndTime);
    sinkApps5.Start(Seconds(0));
    sinkApps5.Stop(simulationEndTime);

    // Configure Application Source On Left Nodes
    applicationInstaller(sourceNodes.Get(0), sinkAddress4, true);
    applicationInstaller(sourceNodes.Get(1), sinkAddress5, true);
    applicationInstaller(sourceNodes.Get(2), sinkAddress5, true);
    applicationInstaller(sourceNodes.Get(3), sinkAddress5, true);
    #pragma endregion

    #pragma region Flow Monitoring And Stats
    FlowMonitorHelper flowmon;
    monitor = flowmon.InstallAll();
    classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    // Simulator::ScheduleNow(&calculateStats, monitor, classifier);
    #pragma endregion

    #pragma region Simulate Network
    Simulator::Stop(simulationEndTime);
    Simulator::Run();

    monitor->CheckForLostPackets();

    Simulator::Destroy();
    #pragma endregion

    return 0;
}