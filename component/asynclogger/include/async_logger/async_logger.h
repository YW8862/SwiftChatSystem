#pragma once

#include "log_level.h"
#include "log_config.h"

#include <string>
#include <sstream>
#include <memory>
#include <vector>
#include <utility>
#include <cstring>

namespace asynclog {

// 前向声明
class AsyncLoggerImpl;

// ============================================================================
// Tag 支持
// ============================================================================

/**
 * @brief 日志标签类
 * 
 * 用于给日志添加结构化的键值对标签
 * @example LogInfo(Tag("user_id", "alice"), "User logged in");
 */
class Tag {
public:
    Tag() = default;
    
    Tag(const std::string& key, const std::string& value) {
        tags_.emplace_back(key, value);
    }
    
    Tag& Add(const std::string& key, const std::string& value) {
        tags_.emplace_back(key, value);
        return *this;
    }
    
    std::string ToString() const {
        if (tags_.empty()) return "";
        std::ostringstream oss;
        bool first = true;
        for (const auto& tag : tags_) {
            if (!first) oss << ", ";
            oss << tag.first << "=" << tag.second;
            first = false;
        }
        return oss.str();
    }
    
    bool Empty() const { return tags_.empty(); }

private:
    std::vector<std::pair<std::string, std::string>> tags_;
};

// ============================================================================
// AsyncLogger 核心类
// ============================================================================

class AsyncLogger {
public:
    static AsyncLogger& Instance();
    
    bool Init(const LogConfig& config = LogConfig{});
    void Shutdown();
    bool IsInitialized() const;
    
    void Log(LogLevel level, const char* file, int line, const std::string& message);
    void Log(LogLevel level, const char* file, int line, const Tag& tag, const std::string& message);
    
    void SetLevel(LogLevel level);
    LogLevel GetLevel() const;
    
    bool ShouldLog(LogLevel level) const {
        return IsInitialized() && level >= GetLevel();
    }
    
    void Flush();

private:
    AsyncLogger();
    ~AsyncLogger();
    AsyncLogger(const AsyncLogger&) = delete;
    AsyncLogger& operator=(const AsyncLogger&) = delete;
    
    std::unique_ptr<AsyncLoggerImpl> impl_;
};

// ============================================================================
// 便捷全局函数
// ============================================================================

inline bool Init(const LogConfig& config = LogConfig{}) {
    return AsyncLogger::Instance().Init(config);
}

inline void Shutdown() {
    AsyncLogger::Instance().Shutdown();
}

inline void SetLevel(LogLevel level) {
    AsyncLogger::Instance().SetLevel(level);
}

inline LogLevel GetLevel() {
    return AsyncLogger::Instance().GetLevel();
}

inline void Flush() {
    AsyncLogger::Instance().Flush();
}

}  // namespace asynclog

// ============================================================================
// 日志宏实现
// ============================================================================

// 提取文件名（去除路径）
#define ASYNCLOG_FILENAME \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : \
     (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__))

// 无 Tag 版本（1个参数）
#define ASYNCLOG_LOG_1(level, msg) \
    do { \
        if (asynclog::AsyncLogger::Instance().ShouldLog(level)) { \
            std::ostringstream oss_; \
            oss_ << msg; \
            asynclog::AsyncLogger::Instance().Log(level, ASYNCLOG_FILENAME, __LINE__, oss_.str()); \
        } \
    } while(0)

// 带 Tag 版本（2+个参数，第一个是 Tag）
#define ASYNCLOG_LOG_N(level, tag, ...) \
    do { \
        if (asynclog::AsyncLogger::Instance().ShouldLog(level)) { \
            std::ostringstream oss_; \
            oss_ << __VA_ARGS__; \
            asynclog::AsyncLogger::Instance().Log(level, ASYNCLOG_FILENAME, __LINE__, tag, oss_.str()); \
        } \
    } while(0)

// 参数个数选择器（支持最多8个参数）
#define ASYNCLOG_GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, NAME, ...) NAME

// 统一日志宏生成器
#define ASYNCLOG_LOG(level, ...) \
    ASYNCLOG_GET_MACRO(__VA_ARGS__, \
        ASYNCLOG_LOG_N, ASYNCLOG_LOG_N, ASYNCLOG_LOG_N, ASYNCLOG_LOG_N, \
        ASYNCLOG_LOG_N, ASYNCLOG_LOG_N, ASYNCLOG_LOG_N, ASYNCLOG_LOG_1 \
    )(level, __VA_ARGS__)

// ============================================================================
// 对外暴露的日志接口宏
// 
// 用法一（无 Tag）：LogInfo("message " << variable);
// 用法二（带 Tag）：LogInfo(TAG("key","val"), "message " << variable);
// ============================================================================

/**
 * @brief TRACE 级别日志
 * @example LogTrace("Processing item " << item_id);
 * @example LogTrace(TAG("id", "123"), "Processing");
 */
#define LogTrace(...) ASYNCLOG_LOG(asynclog::LogLevel::TRACE, __VA_ARGS__)

/**
 * @brief DEBUG 级别日志
 * @example LogDebug("x = " << x);
 * @example LogDebug(TAG("func", "calc"), "result = " << result);
 */
#define LogDebug(...) ASYNCLOG_LOG(asynclog::LogLevel::DEBUG, __VA_ARGS__)

/**
 * @brief INFO 级别日志
 * @example LogInfo("Server started on port " << port);
 * @example LogInfo(TAG("user", "alice"), "User logged in");
 */
#define LogInfo(...) ASYNCLOG_LOG(asynclog::LogLevel::INFO, __VA_ARGS__)

/**
 * @brief WARNING 级别日志
 * @example LogWarning("Connection timeout");
 * @example LogWarning(TAG("conn", "42"), "Retrying...");
 */
#define LogWarning(...) ASYNCLOG_LOG(asynclog::LogLevel::WARN, __VA_ARGS__)

/**
 * @brief ERROR 级别日志
 * @example LogError("Failed: " << error_msg);
 * @example LogError(TAG("code", "E001"), "File not found");
 */
#define LogError(...) ASYNCLOG_LOG(asynclog::LogLevel::ERROR, __VA_ARGS__)

/**
 * @brief FATAL 级别日志
 * @example LogFatal("Critical error!");
 * @example LogFatal(TAG("component", "db"), "Connection lost");
 */
#define LogFatal(...) ASYNCLOG_LOG(asynclog::LogLevel::FATAL, __VA_ARGS__)

// ============================================================================
// 简化的 Tag 创建宏
// ============================================================================

/**
 * @brief 创建 Tag 的便捷宏
 * @example LogInfo(TAG("user", "alice"), "Hello");
 * @example LogInfo(TAG("user", "alice").Add("role", "admin"), "Logged in");
 */
#define TAG(key, value) asynclog::Tag(key, value)
