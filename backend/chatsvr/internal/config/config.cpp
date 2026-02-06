#include "config.h"
#include "swift/config_loader.h"

namespace swift::chat {

/**
 * 从配置文件 + 环境变量加载，填充 ChatConfig。
 * 解析与 env 覆盖由公共库 KeyValueConfig 统一实现，此处仅做「键 → 结构体」映射。
 */
ChatConfig LoadConfig(const std::string& config_file) {
    swift::KeyValueConfig kv = swift::LoadKeyValueConfig(config_file, "CHATSVR_");

    ChatConfig config;
    config.host = kv.Get("host", "0.0.0.0");
    config.port = kv.GetInt("port", 9098);

    config.store_type = kv.Get("store_type", "rocksdb");
    config.rocksdb_path = kv.Get("rocksdb_path", "/data/chat");
    config.mysql_dsn = kv.Get("mysql_dsn", "");

    config.recall_timeout_seconds = kv.GetInt("recall_timeout_seconds", 120);
    config.offline_max_count = kv.GetInt("offline_max_count", 1000);
    config.history_page_size = kv.GetInt("history_page_size", 50);

    config.jwt_secret = kv.Get("jwt_secret", "swift_online_secret_2026");

    config.log_dir = kv.Get("log_dir", "/data/logs");
    config.log_level = kv.Get("log_level", "INFO");

    return config;
}

}  // namespace swift::chat
