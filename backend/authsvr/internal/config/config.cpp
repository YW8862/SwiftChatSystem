#include "config.h"
#include "swift/config_loader.h"

namespace swift::auth {

/**
 * 从配置文件 + 环境变量加载，填充 AuthConfig。
 * 解析与 env 覆盖由公共库 KeyValueConfig 统一实现，此处仅做「键 → 结构体」映射。
 */
AuthConfig LoadConfig(const std::string& config_file) {
    swift::KeyValueConfig kv = swift::LoadKeyValueConfig(config_file, "AUTHSVR_");

    AuthConfig config;
    config.host = kv.Get("host", "0.0.0.0");
    config.port = kv.GetInt("port", 9094);

    config.store_type = kv.Get("store_type", "rocksdb");
    config.rocksdb_path = kv.Get("rocksdb_path", "/data/auth");
    config.mysql_dsn = kv.Get("mysql_dsn", "");

    config.jwt_secret = kv.Get("jwt_secret", "swift_online_secret_2026");

    config.log_dir = kv.Get("log_dir", "/data/logs");
    config.log_level = kv.Get("log_level", "INFO");

    return config;
}

}  // namespace swift::auth
