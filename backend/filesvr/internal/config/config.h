#pragma once

#include <string>

namespace swift::file {

struct FileConfig {
    std::string host = "0.0.0.0";
    int grpc_port = 9100;
    int http_port = 8080;
    
    // 存储配置
    std::string store_type = "rocksdb";
    std::string rocksdb_path = "/data/file-meta";
    
    // 文件存储路径
    std::string storage_path = "/data/files";
    std::string storage_type = "local";  // local / minio / s3
    std::string minio_endpoint;
    std::string minio_access_key;
    std::string minio_secret_key;
    std::string minio_bucket = "swift-files";
    
    // 限制
    int64_t max_file_size = 100 * 1024 * 1024;  // 100MB
    std::string allowed_types = "image/*,video/*,audio/*,application/pdf";
    
    std::string log_dir = "/data/logs";
    std::string log_level = "INFO";
};

FileConfig LoadConfig(const std::string& config_file);

}  // namespace swift::file
