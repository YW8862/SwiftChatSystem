#pragma once

#include "sink.h"
#include "../include/async_logger/log_level.h"
#include <mutex>

namespace asynclog {

/**
 * @brief 控制台输出 Sink
 * 
 * 支持彩色输出（Linux/macOS/Windows 10+）
 */
class ConsoleSink : public Sink {
public:
    /**
     * @brief 构造函数
     * @param enable_color 是否启用彩色输出
     */
    explicit ConsoleSink(bool enable_color = true);
    
    ~ConsoleSink() override = default;
    
    void Write(const std::string& formatted_log) override;
    void Flush() override;
    
    /**
     * @brief 写入带颜色的日志
     * @param level 日志级别（用于确定颜色）
     * @param formatted_log 格式化后的日志
     */
    void WriteWithLevel(LogLevel level, const std::string& formatted_log);

private:
    /**
     * @brief 获取日志级别对应的 ANSI 颜色代码
     */
    const char* GetColorCode(LogLevel level) const;
    
    /**
     * @brief 重置颜色
     */
    const char* GetResetCode() const;
    
    bool enable_color_;
    std::mutex mutex_;
};

}  // namespace asynclog
