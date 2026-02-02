#pragma once

#include "log_level.h"
#include <string>
#include <cstddef>

namespace asynclog {

/**
 * @brief 日志配置结构
 */
struct LogConfig {
    // 日志级别
    LogLevel min_level = LogLevel::INFO;
    
    // 缓冲区配置
    size_t buffer_size = 4 * 1024 * 1024;       // 4MB 环形缓冲区
    size_t flush_interval_ms = 100;              // 刷盘间隔（毫秒）
    
    // 文件输出配置
    bool enable_file = true;
    std::string log_dir = "./logs";              // 日志目录
    std::string file_prefix = "app";             // 文件名前缀
    size_t max_file_size = 100 * 1024 * 1024;   // 100MB 单文件大小上限
    int max_file_count = 10;                     // 保留文件数量
    
    // 控制台输出配置
    bool enable_console = true;
    bool console_color = true;                   // 控制台彩色输出
    
    // 格式配置
    bool show_file_line = true;                  // 显示文件名和行号
    bool show_thread_id = false;                 // 显示线程ID
};

}  // namespace asynclog
