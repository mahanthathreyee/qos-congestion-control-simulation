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
    routers.Create(4);
    sinkNodes.Create(2);

    // Install Internet Stack for IP Address allocation
    InternetStackHelper stack;
    stack.Install(sourceNodes);
    stack.Install(routers);
    stack.Install(sinkNodes);

    // Clear Stats File
    truncateFile();

    Ipv4AddressHelper ipv4;
    #pragma endregion

    #pragma region Configure nodes
    // Left Nodes
    NodeContainer l0c0 = NodeContainer(sourceNodes.Get(0), routers.Get(0));
    NodeContainer l1c0 = NodeContainer(sourceNodes.Get(1), routers.Get(0));
    NodeContainer l2c0 = NodeContainer(sourceNodes.Get(2), routers.Get(0));
    NodeContainer l3c0 = NodeContainer(sourceNodes.Get(3), routers.Get(0));
    // Right Nodes
    NodeContainer r3c0 = NodeContainer(routers.Get(3), sinkNodes.Get(0));
    NodeContainer r3c1 = NodeContainer(routers.Get(3), sinkNodes.Get(1));

    PointToPointHelper nodeRouterLink;
    nodeRouterLink.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
    NetDeviceContainer ld0cd0 = nodeRouterLink.Install(l0c0);
    NetDeviceContainer ld1cd0 = nodeRouterLink.Install(l1c0);
    NetDeviceContainer ld2cd0 = nodeRouterLink.Install(l2c0);
    NetDeviceContainer ld3cd0 = nodeRouterLink.Install(l3c0);
    NetDeviceContainer rd3cd0 = nodeRouterLink.Install(r3c0);
    NetDeviceContainer rd3cd1 = nodeRouterLink.Install(r3c1);

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
    Ipv4InterfaceContainer regLinkInterface_rd3cd0 = ipv4.Assign(rd3cd0);
    ipv4.SetBase("10.2.2.0","255.255.255.0");
    Ipv4InterfaceContainer regLinkInterface_rd3cd1 = ipv4.Assign(rd3cd1);
    #pragma endregion

    #pragma region Configure routers
    PointToPointHelper bottleNeckLinkA;
    bottleNeckLinkA.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
    NodeContainer n0n1 = NodeContainer(routers.Get(0), routers.Get(1));
    NetDeviceContainer cd0cd1 = bottleNeckLinkA.Install(n0n1);
    ipv4.SetBase("10.3.1.0","255.255.255.0");
    Ipv4InterfaceContainer bottleneckInterfaceA = ipv4.Assign(cd0cd1);

    PointToPointHelper bottleNeckLinkB;
    bottleNeckLinkB.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
    NodeContainer n1n3 = NodeContainer(routers.Get(1), routers.Get(3));
    NetDeviceContainer cd1cd3 = bottleNeckLinkB.Install(n1n3);
    ipv4.SetBase("10.3.2.0","255.255.255.0");
    Ipv4InterfaceContainer bottleneckInterfaceB = ipv4.Assign(cd1cd3);

    PointToPointHelper bottleNeckLinkC;
    bottleNeckLinkC.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
    NodeContainer n0n2 = NodeContainer(routers.Get(0), routers.Get(2));
    NetDeviceContainer cd0cd2 = bottleNeckLinkC.Install(n0n2);
    ipv4.SetBase("10.3.3.0","255.255.255.0");
    Ipv4InterfaceContainer bottleneckInterfaceC = ipv4.Assign(cd0cd1);

    PointToPointHelper bottleNeckLinkD;
    bottleNeckLinkD.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
    NodeContainer n2n3 = NodeContainer(routers.Get(2), routers.Get(3));
    NetDeviceContainer cd2cd3 = bottleNeckLinkD.Install(n2n3);
    ipv4.SetBase("10.3.4.0","255.255.255.0");
    Ipv4InterfaceContainer bottleneckInterfaceD = ipv4.Assign(cd2cd3);
    #pragma endregion Configure routers

    #pragma region Utility Helper 
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    #pragma endregion

    #pragma region Configure Application 
    // Configure Application Sink On Right Nodes
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
    Address sinkAddress4(
        InetSocketAddress(regLinkInterface_rd3cd0.GetAddress(1), sinkPort));
    Address sinkAddress5(
        InetSocketAddress(regLinkInterface_rd3cd1.GetAddress(1), sinkPort));
    applicationSinkInstaller(sinkNodes.Get(0));
    applicationSinkInstaller(sinkNodes.Get(1));

    // Configure Application Source On Left Nodes
    applicationSourceInstaller(sourceNodes.Get(0), sinkAddress4, true);
    applicationSourceInstaller(sourceNodes.Get(1), sinkAddress4, true);
    applicationSourceInstaller(sourceNodes.Get(2), sinkAddress5, true);
    applicationSourceInstaller(sourceNodes.Get(3), sinkAddress5, true);
    
    Simulator::Schedule (Seconds(10), &Ipv4::SetDown, bottleneckInterfaceB.Get(0).first, 2);
    Simulator::Schedule (Seconds(10), &Ipv4::SetDown, bottleneckInterfaceB.Get(0).first, 1);

    Simulator::Schedule (Seconds(10), &Ipv4::SetDown, bottleneckInterfaceC.Get(1).first, 2);
    Simulator::Schedule (Seconds(10), &Ipv4::SetDown, bottleneckInterfaceC.Get(1).first, 1);
    
    Simulator::Schedule (Seconds(40), &Ipv4::SetUp, bottleneckInterfaceB.Get(0).first, 2);
    Simulator::Schedule (Seconds(40), &Ipv4::SetUp, bottleneckInterfaceB.Get(0).first, 1);
    #pragma endregion

    #pragma region Flow Monitoring And Stats
    FlowMonitorHelper flowmon;
    monitor = flowmon.InstallAll();
    classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    #pragma endregion

    #pragma region Simulate Network
    Simulator::Stop(simulationEndTime);
    Simulator::Run();

    Simulator::Destroy();
    #pragma endregion

    return 0;
}