#pragma once

#include <string>

namespace swift::auth {

/**
 * AuthSvr 配置
 *
 * 加载顺序：默认值 -> 配置文件（若存在）-> 环境变量（覆盖）
 *
 * 环境变量（可选覆盖）：
 *   AUTHSVR_CONFIG      配置文件路径（main 未传参时使用）
 *   AUTHSVR_HOST        监听地址，默认 0.0.0.0
 *   AUTHSVR_PORT        监听端口，默认 9094
 *   AUTHSVR_STORE_TYPE  存储类型：rocksdb / mysql
 *   AUTHSVR_ROCKSDB_PATH RocksDB 数据目录
 *   AUTHSVR_LOG_DIR     日志目录
 *   AUTHSVR_LOG_LEVEL   日志级别：TRACE/DEBUG/INFO/WARNING/ERROR
 */
struct AuthConfig {
  // 服务配置
  std::string host = "0.0.0.0";
  int port = 9094;

  // 存储配置
  std::string store_type = "rocksdb"; // rocksdb / mysql
  std::string rocksdb_path = "/data/auth";
  std::string mysql_dsn;

  // 日志配置
  std::string log_dir = "/data/logs";
  std::string log_level = "INFO";
};

/**
 * 加载配置
 * @param config_file 配置文件路径，支持 key=value 格式（#
 * 注释）；若文件不存在则仅用默认值 + 环境变量
 * @return 合并后的配置
 */
AuthConfig LoadConfig(const std::string &config_file);

} // namespace swift::auth
