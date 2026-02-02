#include "console_sink.h"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace asynclog {

ConsoleSink::ConsoleSink(bool enable_color)
    : enable_color_(enable_color) {
    
#ifdef _WIN32
    // Windows 10+ 需要启用 ANSI 转义序列支持
    if (enable_color_) {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE) {
            DWORD dwMode = 0;
            if (GetConsoleMode(hOut, &dwMode)) {
                dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(hOut, dwMode);
            }
        }
    }
#endif
}

void ConsoleSink::Write(const std::string& formatted_log) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << formatted_log;
}

void ConsoleSink::WriteWithLevel(LogLevel level, const std::string& formatted_log) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (enable_color_) {
        std::cout << GetColorCode(level) << formatted_log << GetResetCode();
    } else {
        std::cout << formatted_log;
    }
}

void ConsoleSink::Flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout.flush();
}

const char* ConsoleSink::GetColorCode(LogLevel level) const {
    switch (level) {
        case LogLevel::TRACE: return "\033[37m";       // 白色
        case LogLevel::DEBUG: return "\033[36m";       // 青色
        case LogLevel::INFO:  return "\033[32m";       // 绿色
        case LogLevel::WARN:  return "\033[33m";       // 黄色
        case LogLevel::ERROR: return "\033[31m";       // 红色
        case LogLevel::FATAL: return "\033[35;1m";     // 紫色加粗
        default:              return "\033[0m";        // 默认
    }
}

const char* ConsoleSink::GetResetCode() const {
    return "\033[0m";
}

}  // namespace asynclog
