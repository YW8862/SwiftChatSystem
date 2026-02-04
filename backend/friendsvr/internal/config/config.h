#pragma once

#include <string>

namespace swift::friend_ {

/**
 * FriendSvr 配置
 *
 * 加载顺序：默认值 -> 配置文件（若存在）-> 环境变量（覆盖）
 *
 * 环境变量（可选覆盖）：
 *   FRIENDSVR_CONFIG       配置文件路径（main 未传参时使用）
 *   FRIENDSVR_HOST         监听地址，默认 0.0.0.0
 *   FRIENDSVR_PORT         监听端口，默认 9096
 *   FRIENDSVR_STORE_TYPE   存储类型：rocksdb / mysql
 *   FRIENDSVR_ROCKSDB_PATH RocksDB 数据目录
 *   FRIENDSVR_MYSQL_DSN    MySQL DSN（store_type=mysql 时）
 *   FRIENDSVR_LOG_DIR      日志目录
 *   FRIENDSVR_LOG_LEVEL    日志级别：TRACE/DEBUG/INFO/WARNING/ERROR
 *   FRIENDSVR_JWT_SECRET    JWT 校验密钥（与 OnlineSvr 签发 Token 一致，用于校验请求身份）
 */
struct FriendConfig {
    std::string host = "0.0.0.0";
    int port = 9096;

    std::string store_type = "rocksdb";
    std::string rocksdb_path = "/data/friend";
    std::string mysql_dsn;

    /** 与 OnlineSvr 相同的 JWT 密钥，用于从 metadata 校验 Token 得到 user_id */
    std::string jwt_secret = "swift_online_secret_2026";

    std::string log_dir = "/data/logs";
    std::string log_level = "INFO";
};

/** 加载配置：支持 key=value 配置文件，缺失时用默认值，环境变量覆盖 */
FriendConfig LoadConfig(const std::string& config_file);

}  // namespace swift::friend_
