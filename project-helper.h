#include "config.h"

using namespace ns3;

Ptr<FlowMonitor> monitor;
Ptr<Ipv4FlowClassifier> classifier;

struct NodeMetrics {
    std::vector<double> throughput;
    std::vector<size_t> packetsDropped;
};

NodeMetrics calculateMetrics(uint32_t nodeId) {
    NodeMetrics nodeMetrics;
    
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter;
    
    for (iter = stats.begin(); iter != stats.end(); iter++) {
        uint32_t pktDropSum = 0;
        std::vector<uint32_t> pktDropVector = iter->second.packetsDropped;
        for (auto tIter = pktDropVector.begin(); tIter != pktDropVector.end(); tIter++) {
            pktDropSum += *tIter;
        }

        double current_time = Simulator::Now().GetSeconds();
        double transferredBytes = (iter->second.rxBytes * 8.0);
        double throughput = transferredBytes / current_time / 1000 / 1000;
        
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

void windowSizeChangeCallback(uint32_t nodeId, uint32_t oldCwnd, uint32_t newCwnd) {
    NodeMetrics nodeMetrics = calculateMetrics(nodeId);
    std::ofstream fPlotQueue(queueStatFile, std::ios::out | std::ios::app);
    Ipv4FlowClassifier::FiveTuple packet = classifier->FindFlow(nodeId + 1);

    fPlotQueue  << nodeId 
                << " (" << packet.sourceAddress << " -> " << packet.destinationAddress << ")" 
                << ","
                << Simulator::Now().GetSeconds() << "," 
                << nodeMetrics.throughput[nodeId] << ","
                << newCwnd / segmentSize << ","
                << nodeMetrics.packetsDropped[nodeId] << ","
                << calculateJainFairness(nodeMetrics.throughput) << std::endl;
    fPlotQueue.close();
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

void applicationInstaller(Ptr<Node> node, Address sinkAddress) {
    BulkSendHelper source("ns3::TcpSocketFactory", sinkAddress);
    source.SetAttribute("MaxBytes", UintegerValue(maxBytes));

    ApplicationContainer sourceApps = source.Install(node);
    sourceApps.Start(MicroSeconds(100));
    sourceApps.Stop(simulationEndTime);

    Callback<void, uint32_t, uint32_t> callback;
    uint32_t nodeId = node->GetId();

    Simulator::Schedule(Seconds(0.5) + Seconds(0.001), 
        &registerWindowSizeTracer, nodeId);
}

void truncateFile() {
    std::ofstream fPlotQueue(std::stringstream(queueStatFile).str(),
                            std::ios::out | std::ios::trunc);
    fPlotQueue  << "Flow ID,Time,Throughput,Window Size,Packets Dropped,Fairness\n";
    fPlotQueue.close();
}