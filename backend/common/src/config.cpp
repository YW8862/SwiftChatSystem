#include "swift/config.h"
#include "swift/config_loader.h"

namespace swift {

bool LoadConfig(const std::string& path, ServiceConfig& config) {
    KeyValueConfig kv = LoadKeyValueConfig(path, "");
    config.service_name = kv.Get("service_name", config.service_name);
    config.host = kv.Get("host", config.host);
    config.grpc_port = static_cast<uint16_t>(kv.GetInt("grpc_port", config.grpc_port));
    config.log_dir = kv.Get("log_dir", config.log_dir);
    config.log_level = kv.Get("log_level", config.log_level);
    config.data_dir = kv.Get("data_dir", config.data_dir);
    return true;
}

}  // namespace swift
