#include "log_formatter.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>

namespace asynclog {

LogFormatter::LogFormatter(bool show_file_line, bool show_thread_id)
    : show_file_line_(show_file_line)
    , show_thread_id_(show_thread_id) {
}

std::string LogFormatter::Format(const LogEntry& entry) const {
    std::ostringstream oss;
    
    // 时间戳
    oss << "[" << FormatTimestamp(entry.timestamp) << "] ";
    
    // 日志级别
    oss << "[" << LogLevelToString(static_cast<LogLevel>(entry.level)) << "] ";
    
    // 线程ID（可选）
    if (show_thread_id_) {
        oss << "[tid:" << std::this_thread::get_id() << "] ";
    }
    
    // 标签（如果有）
    if (!entry.tags.empty()) {
        oss << "[" << entry.tags << "] ";
    }
    
    // 文件名和行号（可选）
    if (show_file_line_ && !entry.file.empty()) {
        oss << "[" << entry.file << ":" << entry.line << "] ";
    }
    
    // 消息内容
    oss << entry.message << "\n";
    
    return oss.str();
}

int64_t LogFormatter::GetCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

std::string LogFormatter::FormatTimestamp(int64_t timestamp) {
    // 转换为秒和微秒
    auto seconds = timestamp / 1000000;
    auto micros = timestamp % 1000000;
    
    // 转换为时间结构
    std::time_t time = static_cast<std::time_t>(seconds);
    std::tm tm_buf;
    
#ifdef _WIN32
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif
    
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S")
        << "." << std::setfill('0') << std::setw(3) << (micros / 1000);
    
    return oss.str();
}

}  // namespace asynclog
