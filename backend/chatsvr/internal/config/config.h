#pragma once

#include <string>

namespace swift::chat {

struct ChatConfig {
    std::string host = "0.0.0.0";
    int port = 9098;
    
    // 存储配置
    std::string store_type = "rocksdb";
    std::string rocksdb_path = "/data/chat";
    std::string mysql_dsn;
    
    // 消息配置
    int recall_timeout_seconds = 120;
    int offline_max_count = 1000;
    int history_page_size = 50;
    
    // 日志
    std::string log_dir = "/data/logs";
    std::string log_level = "INFO";
};

ChatConfig LoadConfig(const std::string& config_file);

}  // namespace swift::chat
