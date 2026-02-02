#pragma once

#include <string>

namespace swift::gate {

struct GateConfig {
    std::string host = "0.0.0.0";
    int websocket_port = 9090;
    int grpc_port = 9091;      // 供 ZoneSvr 调用
    
    // ZoneSvr 地址
    std::string zone_svr_addr = "zonesvr:9092";
    
    // 连接配置
    int max_connections = 10000;
    int heartbeat_interval_seconds = 30;
    int heartbeat_timeout_seconds = 90;
    
    std::string log_dir = "/data/logs";
    std::string log_level = "INFO";
};

GateConfig LoadConfig(const std::string& config_file);

}  // namespace swift::gate
