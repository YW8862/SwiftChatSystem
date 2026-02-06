#pragma once

/**
 * FileSvr 配置结构：字段由 LoadConfig 从配置文件 + 环境变量（FILESVR_*）填充。
 * 解析逻辑在公共库 swift::KeyValueConfig，此处仅定义结构及缺省值。
 */
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
    
    // 限制（单文件上限，发送与接收均不可超过）
    int64_t max_file_size = 1024LL * 1024 * 1024;  // 默认 1GB
    std::string allowed_types = "image/*,video/*,audio/*,application/pdf";
    // 上传会话过期时间（秒）；超时未续传完成则放弃上传、删除已传数据，并通知 ChatSvr 将关联消息标为发送失败
    int64_t upload_session_expire_seconds = 24 * 3600;  // 默认 24 小时
    
    std::string log_dir = "/data/logs";
    std::string log_level = "INFO";

    // 与 OnlineSvr 一致，用于从 gRPC metadata 校验 Token 得到当前 user_id；空则使用请求体中的 user_id
    std::string jwt_secret;
};

FileConfig LoadConfig(const std::string& config_file);

}  // namespace swift::file
