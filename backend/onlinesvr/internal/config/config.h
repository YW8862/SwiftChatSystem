#pragma once

#include <string>

namespace swift::online {

struct OnlineConfig {
    std::string host = "0.0.0.0";
    int port = 9095;

    std::string store_type = "rocksdb";
    std::string rocksdb_path = "/data/online";

    std::string jwt_secret = "swift_online_secret_2026";
    int jwt_expire_hours = 24 * 7;  // 7 天

    std::string log_dir = "/data/logs";
    std::string log_level = "INFO";
};

OnlineConfig LoadConfig(const std::string& config_file);

}  // namespace swift::online
