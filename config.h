#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

using namespace ns3;

uint128_t maxBytes = ((uint128_t) 5) * ((uint128_t) 1024) * ((uint128_t) 1024) * ((uint128_t) 1024);
uint16_t sinkPort = 8080;
uint32_t segmentSize = 536;
Time simulationEndTime = Seconds(3600);
std::string transportProtocol = "ns3::TcpCubic";
std::string flowStatFile = "results/flow_stats.csv";