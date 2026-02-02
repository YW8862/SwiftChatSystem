#pragma once

#include <string>
#include <cstdint>

namespace swift {

/**
 * 服务配置基类
 */
struct ServiceConfig {
    std::string service_name;
    std::string host = "0.0.0.0";
    uint16_t grpc_port = 9090;
    
    // 日志配置
    std::string log_dir = "./logs";
    std::string log_level = "INFO";
    
    // RocksDB 配置（如果需要）
    std::string data_dir = "./data";
};

/**
 * 加载配置文件
 */
bool LoadConfig(const std::string& path, ServiceConfig& config);

}  // namespace swift
