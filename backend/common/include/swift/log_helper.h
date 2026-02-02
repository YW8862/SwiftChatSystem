#pragma once

/**
 * @file log_helper.h
 * @brief SwiftChatSystem 日志封装层
 * 
 * 完全封装底层日志库，其他模块只需引用此文件。
 * 如果将来更换日志库，只需修改此文件。
 */

#include <async_logger/async_logger.h>  // 底层实现，仅此处引用

#include <cstdlib>
#include <string>

namespace swift {
namespace log {

// ============================================================================
// 日志级别（与底层解耦）
// ============================================================================
enum class Level {
    TRACE = 0,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

// ============================================================================
// 日志配置
// ============================================================================
struct Config {
    std::string log_dir = "./logs";
    std::string file_prefix = "app";
    Level min_level = Level::INFO;
    bool enable_console = true;
    bool enable_file = true;
    bool show_file_line = true;
    bool console_color = true;
    size_t max_file_size = 100 * 1024 * 1024;  // 100MB
    int max_file_count = 10;
};

// ============================================================================
// 日志初始化与关闭
// ============================================================================

/**
 * @brief 初始化日志系统
 * @param config 日志配置
 * @return 是否成功
 */
inline bool Init(const Config& config) {
    asynclog::LogConfig impl_config;
    impl_config.log_dir = config.log_dir;
    impl_config.file_prefix = config.file_prefix;
    impl_config.enable_console = config.enable_console;
    impl_config.enable_file = config.enable_file;
    impl_config.show_file_line = config.show_file_line;
    impl_config.console_color = config.console_color;
    impl_config.max_file_size = config.max_file_size;
    impl_config.max_file_count = config.max_file_count;
    
    // 转换日志级别
    switch (config.min_level) {
        case Level::TRACE: impl_config.min_level = asynclog::LogLevel::TRACE; break;
        case Level::DEBUG: impl_config.min_level = asynclog::LogLevel::DEBUG; break;
        case Level::INFO:  impl_config.min_level = asynclog::LogLevel::INFO;  break;
        case Level::WARN:  impl_config.min_level = asynclog::LogLevel::WARN;  break;
        case Level::ERROR: impl_config.min_level = asynclog::LogLevel::ERROR; break;
        case Level::FATAL: impl_config.min_level = asynclog::LogLevel::FATAL; break;
    }
    
    return asynclog::Init(impl_config);
}

/**
 * @brief 从环境变量初始化日志（适合容器部署）
 * 
 * 环境变量：
 *   LOG_DIR     - 日志目录，默认 ./logs
 *   LOG_LEVEL   - 日志级别，默认 INFO
 *   LOG_CONSOLE - 是否输出到控制台，默认 true
 */
inline bool InitFromEnv(const std::string& service_name) {
    Config config;
    config.file_prefix = service_name;
    
    if (const char* log_dir = std::getenv("LOG_DIR")) {
        config.log_dir = log_dir;
    }
    
    if (const char* log_level = std::getenv("LOG_LEVEL")) {
        std::string level = log_level;
        if (level == "TRACE") config.min_level = Level::TRACE;
        else if (level == "DEBUG") config.min_level = Level::DEBUG;
        else if (level == "INFO") config.min_level = Level::INFO;
        else if (level == "WARN") config.min_level = Level::WARN;
        else if (level == "ERROR") config.min_level = Level::ERROR;
        else if (level == "FATAL") config.min_level = Level::FATAL;
    }
    
    if (const char* log_console = std::getenv("LOG_CONSOLE")) {
        config.enable_console = (std::string(log_console) != "false");
    }
    
    return Init(config);
}

/**
 * @brief 关闭日志系统，确保所有日志写入完成
 */
inline void Shutdown() {
    asynclog::Shutdown();
}

/**
 * @brief 动态修改日志级别
 */
inline void SetLevel(Level level) {
    asynclog::LogLevel impl_level;
    switch (level) {
        case Level::TRACE: impl_level = asynclog::LogLevel::TRACE; break;
        case Level::DEBUG: impl_level = asynclog::LogLevel::DEBUG; break;
        case Level::INFO:  impl_level = asynclog::LogLevel::INFO;  break;
        case Level::WARN:  impl_level = asynclog::LogLevel::WARN;  break;
        case Level::ERROR: impl_level = asynclog::LogLevel::ERROR; break;
        case Level::FATAL: impl_level = asynclog::LogLevel::FATAL; break;
    }
    asynclog::SetLevel(impl_level);
}

}  // namespace log
}  // namespace swift

// ============================================================================
// 日志宏（直接暴露，使用方便）
// 
// 用法：
//   SwiftLogInfo("Server started on port " << port);
//   SwiftLogError(TAG("code", "500"), "Internal error");
// ============================================================================

#define SwiftLogTrace(...)   LogTrace(__VA_ARGS__)
#define SwiftLogDebug(...)   LogDebug(__VA_ARGS__)
#define SwiftLogInfo(...)    LogInfo(__VA_ARGS__)
#define SwiftLogWarning(...) LogWarning(__VA_ARGS__)
#define SwiftLogError(...)   LogError(__VA_ARGS__)
#define SwiftLogFatal(...)   LogFatal(__VA_ARGS__)

// 也可以直接使用底层宏（如果不想加 Swift 前缀）
// LogInfo, LogDebug, LogError 等已由 async_logger.h 提供
