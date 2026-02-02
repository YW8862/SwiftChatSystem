#pragma once

#include "ring_buffer.h"
#include "../include/async_logger/log_level.h"
#include <string>

namespace asynclog {

/**
 * @brief 日志格式化器
 * 
 * 将 LogEntry 格式化为可输出的字符串
 */
class LogFormatter {
public:
    /**
     * @brief 构造函数
     * @param show_file_line 是否显示文件名和行号
     * @param show_thread_id 是否显示线程ID
     */
    LogFormatter(bool show_file_line = true, bool show_thread_id = false);
    
    /**
     * @brief 格式化日志条目
     * @param entry 日志条目
     * @return 格式化后的字符串
     */
    std::string Format(const LogEntry& entry) const;
    
    /**
     * @brief 获取当前时间戳（微秒）
     */
    static int64_t GetCurrentTimestamp();
    
    /**
     * @brief 格式化时间戳
     * @param timestamp 微秒时间戳
     * @return 格式化的时间字符串
     */
    static std::string FormatTimestamp(int64_t timestamp);

private:
    bool show_file_line_;
    bool show_thread_id_;
};

}  // namespace asynclog
