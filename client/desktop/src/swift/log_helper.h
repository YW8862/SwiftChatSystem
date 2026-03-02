#pragma once

/**
 * @file log_helper.h
 * @brief SwiftChat 客户端日志封装层（客户端本地副本）
 *
 * 说明：该文件从后端同名封装拷贝而来，客户端独立维护。
 * 前后端不要互相包含头文件，避免构建耦合。
 */

#include <async_logger/async_logger.h>

#include <cstdlib>
#include <string>

namespace swift {
namespace log {

enum class Level {
    TRACE = 0,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

struct Config {
    std::string log_dir = "./logs";
    std::string file_prefix = "app";
    Level min_level = Level::INFO;
    bool enable_console = true;
    bool enable_file = true;
    bool show_file_line = true;
    bool console_color = true;
    size_t max_file_size = 100 * 1024 * 1024;
    int max_file_count = 10;
};

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

inline void Shutdown() {
    asynclog::Shutdown();
}

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

#define SwiftLogTrace(...)   LogTrace(__VA_ARGS__)
#define SwiftLogDebug(...)   LogDebug(__VA_ARGS__)
#define SwiftLogInfo(...)    LogInfo(__VA_ARGS__)
#define SwiftLogWarning(...) LogWarning(__VA_ARGS__)
#define SwiftLogError(...)   LogError(__VA_ARGS__)
#define SwiftLogFatal(...)   LogFatal(__VA_ARGS__)
