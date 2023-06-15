#include "config.h"

using namespace ns3;

Ptr<FlowMonitor> monitor;
Ptr<Ipv4FlowClassifier> classifier;
double currentCWSize = 1;

#pragma region Calculate and Store Flow Metrics
struct NodeMetrics {
    std::vector<double> throughput;
    std::vector<size_t> packetsDropped;
};

NodeMetrics calculateMetrics() {
    NodeMetrics nodeMetrics;
    
    monitor->CheckForLostPackets();
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter;
    
    for (iter = stats.begin(); iter != stats.end(); iter++) {
        uint32_t pktDropSum = 0;
        std::vector<uint32_t> pktDropVector = iter->second.packetsDropped;
        for (auto tIter = pktDropVector.begin(); tIter != pktDropVector.end(); tIter++) {
            pktDropSum += *tIter;
        }

        double current_time = iter->second.timeLastTxPacket.GetSeconds() - iter->second.timeFirstTxPacket.GetSeconds();
        double transferredBytes = (iter->second.txBytes * 8.0);
        double throughput = transferredBytes / (current_time * 1024 * 1024);
        
        nodeMetrics.packetsDropped.push_back(pktDropSum);
        nodeMetrics.throughput.push_back(throughput);
    }

    return nodeMetrics;
}

double calculateJainFairness(const std::vector<double>& throughputs) {
    double sum = 0.0;
    double sumSquared = 0.0;
    double fairnessIndex = 0.0;
    int numFlows = throughputs.size();

    for (int i = 0; i < numFlows; ++i) {
        sum += throughputs[i];
        sumSquared += throughputs[i] * throughputs[i];
    }

    if (sum != 0.0) {
        fairnessIndex = ((sum * sum) / (numFlows * sumSquared));
    }

    return fairnessIndex;
}

void storeMetrics(uint32_t nodeId) {
    NodeMetrics nodeMetrics = calculateMetrics();
    std::ofstream fPlotFlow(flowStatFile, std::ios::out | std::ios::app);

    Simulator::Schedule(simulationMetricInterval, &storeMetrics, nodeId);

    fPlotFlow   << nodeId << ","
                << Simulator::Now().GetSeconds() << "," 
                << nodeMetrics.throughput[nodeId] << ","
                << nodeMetrics.packetsDropped[nodeId] << ","
                << calculateJainFairness(nodeMetrics.throughput) << ","
                << currentCWSize << std::endl;
    fPlotFlow.close();
}
#pragma endregion 

#pragma region Store Congestion Window Change
void windowSizeChangeCallback(uint32_t nodeId, uint32_t oldCwnd, uint32_t newCwnd) {
    currentCWSize = newCwnd / segmentSize;
}

void registerWindowSizeTracer(uint32_t nodeId) {
    std::string cwndTraceLoc = "/NodeList/" 
                                + std::to_string(nodeId) 
                                + "/$ns3::TcpL4Protocol/SocketList/" 
                                + std::to_string(0) 
                                + "/CongestionWindow";
    
    Config::ConnectWithoutContext(cwndTraceLoc, 
        MakeBoundCallback(&windowSizeChangeCallback, nodeId));
}
#pragma endregion

#pragma region Node Application Installer
void applicationSourceInstaller(Ptr<Node> node, Address sinkAddress, bool bulkApplication) {
    ApplicationContainer sourceApps;
    
    if (bulkApplication) {
        BulkSendHelper source("ns3::TcpSocketFactory", sinkAddress);
        source.SetAttribute("MaxBytes", UintegerValue(maxBytes));
        sourceApps = source.Install(node);
    }
    
    sourceApps.Start(Seconds(0.1));
    sourceApps.Stop(simulationEndTime);
    
    Callback<void, uint32_t, uint32_t> callback;
    uint32_t nodeId = node->GetId();

    Simulator::Schedule(Seconds(0.1) + Seconds(0.001), 
        &registerWindowSizeTracer, nodeId);
    Simulator::Schedule(Seconds(0.1) + Seconds(0.001), 
        &storeMetrics, nodeId);
}

void applicationSinkInstaller(Ptr<Node> sinkNodes) {
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
    ApplicationContainer sinkApps = packetSinkHelper.Install(sinkNodes);
    sinkApps.Start(Seconds(0));
    sinkApps.Stop(simulationEndTime);
}
#pragma endregion

void truncateFile() {
    std::ofstream fPlotFlow(std::stringstream(flowStatFile).str(),
                            std::ios::out | std::ios::trunc);
    fPlotFlow  << "Flow ID,Time,Throughput,Packets Dropped,Fairness,Window Size" << std::endl;
    fPlotFlow.close();
}
