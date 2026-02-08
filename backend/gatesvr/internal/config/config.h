#pragma once

#include <string>

namespace swift::gate {

struct GateConfig {
    std::string host = "0.0.0.0";
    int websocket_port = 9090;
    int grpc_port = 9091;      // 供 ZoneSvr 调用

    /** 本 Gate 唯一 ID，多实例时必填或依赖 fallback；为空时使用 hostname:grpc_port */
    std::string gate_id;

    // ZoneSvr 地址
    std::string zone_svr_addr = "localhost:9092";

    // 调用 ZoneSvr 时携带的内网密钥，与 ZoneSvr ZONESVR_INTERNAL_SECRET 一致；建议仅从环境变量 GATESVR_ZONESVR_INTERNAL_SECRET 注入
    std::string zonesvr_internal_secret;
    
    // 连接配置
    int max_connections = 10000;
    int heartbeat_interval_seconds = 30;
    int heartbeat_timeout_seconds = 90;
    
    std::string log_dir = "/data/logs";
    std::string log_level = "INFO";
};

GateConfig LoadConfig(const std::string& config_file);

}  // namespace swift::gate
