#pragma once

#include <string>

namespace swift::zone {

struct ZoneConfig {
    std::string host = "0.0.0.0";
    int port = 9092;
    
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
