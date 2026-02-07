#pragma once

#include <string>

namespace swift::zone {

struct ZoneConfig {
    std::string host = "0.0.0.0";
    int port = 9092;

    // 后端服务地址（RPC 转发用）
    std::string auth_svr_addr = "localhost:9094";
    std::string online_svr_addr = "localhost:9095";
    std::string friend_svr_addr = "localhost:9096";
    std::string chat_svr_addr = "localhost:9098";
    std::string file_svr_addr = "localhost:9100";
    std::string gate_svr_addr = "localhost:9091";  // 推送消息到 Gate 时用

    // 会话存储配置
    std::string session_store_type = "memory";  // memory / redis
    std::string redis_url = "redis://localhost:6379";

    // 会话过期时间（秒）
    int session_expire_seconds = 3600;

    // Gate 心跳超时（秒）
    int gate_heartbeat_timeout = 30;

    std::string log_dir = "/data/logs";
    std::string log_level = "INFO";
};

ZoneConfig LoadConfig(const std::string& config_file);

}  // namespace swift::zone
