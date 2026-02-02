#pragma once

#include <string>

namespace swift::auth {

struct AuthConfig {
    // 服务配置
    std::string host = "0.0.0.0";
    int port = 9094;
    
    // 存储配置
    std::string store_type = "rocksdb";   // rocksdb / mysql
    std::string rocksdb_path = "/data/auth";
    std::string mysql_dsn;
    
    // JWT 配置
    std::string jwt_secret = "your-secret-key";
    int jwt_expire_hours = 24 * 7;  // 7 天
    
    // 日志配置
    std::string log_dir = "/data/logs";
    std::string log_level = "INFO";
};

AuthConfig LoadConfig(const std::string& config_file);

}  // namespace swift::auth
