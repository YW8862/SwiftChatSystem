#pragma once

#include <string>

namespace swift::friend_ {

struct FriendConfig {
    std::string host = "0.0.0.0";
    int port = 9096;
    
    std::string store_type = "rocksdb";
    std::string rocksdb_path = "/data/friend";
    std::string mysql_dsn;
    
    std::string log_dir = "/data/logs";
    std::string log_level = "INFO";
};

FriendConfig LoadConfig(const std::string& config_file);

}  // namespace swift::friend_
