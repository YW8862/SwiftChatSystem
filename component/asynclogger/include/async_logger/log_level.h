#pragma once

#include <string>

namespace asynclog {

/**
 * @brief 日志级别枚举
 */
enum class LogLevel {
    TRACE = 0,   // 最详细的跟踪信息
    DEBUG = 1,   // 调试信息
    INFO  = 2,   // 一般信息
    WARN  = 3,   // 警告信息
    ERROR = 4,   // 错误信息
    FATAL = 5,   // 致命错误
    OFF   = 6    // 关闭日志
};

/**
 * @brief 将日志级别转换为字符串
 */
inline const char* LogLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        case LogLevel::OFF:   return "OFF";
        default:              return "UNKNOWN";
    }
}

/**
 * @brief 将字符串转换为日志级别
 */
inline LogLevel StringToLogLevel(const std::string& str) {
    if (str == "TRACE" || str == "trace") return LogLevel::TRACE;
    if (str == "DEBUG" || str == "debug") return LogLevel::DEBUG;
    if (str == "INFO"  || str == "info")  return LogLevel::INFO;
    if (str == "WARN"  || str == "warn")  return LogLevel::WARN;
    if (str == "ERROR" || str == "error") return LogLevel::ERROR;
    if (str == "FATAL" || str == "fatal") return LogLevel::FATAL;
    if (str == "OFF"   || str == "off")   return LogLevel::OFF;
    return LogLevel::INFO;  // 默认
}

}  // namespace asynclog
