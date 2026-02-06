#include "config.h"
#include "swift/config_loader.h"

namespace swift::file {

/**
 * 从配置文件 + 环境变量加载，填充 FileConfig。
 * 配置项完全由文件/环境提供，此处仅做「键名 → 结构体字段」的映射与缺省值。
 */
FileConfig LoadConfig(const std::string& config_file) {
    swift::KeyValueConfig kv = swift::LoadKeyValueConfig(config_file, "FILESVR_");

    FileConfig config;
    config.host = kv.Get("host", "0.0.0.0");
    config.grpc_port = kv.GetInt("grpc_port", 9100);
    config.http_port = kv.GetInt("http_port", 8080);

    config.store_type = kv.Get("store_type", "rocksdb");
    config.rocksdb_path = kv.Get("rocksdb_path", "/data/file-meta");

    config.storage_path = kv.Get("storage_path", "/data/files");
    config.storage_type = kv.Get("storage_type", "local");
    config.minio_endpoint = kv.Get("minio_endpoint", "");
    config.minio_access_key = kv.Get("minio_access_key", "");
    config.minio_secret_key = kv.Get("minio_secret_key", "");
    config.minio_bucket = kv.Get("minio_bucket", "swift-files");

    config.max_file_size = kv.GetInt64("max_file_size", 1024LL * 1024 * 1024);
    config.allowed_types = kv.Get("allowed_types", "image/*,video/*,audio/*,application/pdf");
    config.upload_session_expire_seconds = kv.GetInt64("upload_session_expire_seconds", 24 * 3600);

    config.log_dir = kv.Get("log_dir", "/data/logs");
    config.log_level = kv.Get("log_level", "INFO");
    config.jwt_secret = kv.Get("jwt_secret", "");

    return config;
}

}  // namespace swift::file
