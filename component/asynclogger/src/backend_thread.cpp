#include "backend_thread.h"
#include "console_sink.h"

namespace asynclog {

BackendThread::BackendThread(RingBuffer& buffer, const LogConfig& config)
    : buffer_(buffer)
    , config_(config)
    , formatter_(config.show_file_line, config.show_thread_id) {
}

BackendThread::~BackendThread() {
    Stop();
}

void BackendThread::Start() {
    if (running_.load()) {
        return;
    }
    
    running_.store(true);
    thread_ = std::thread(&BackendThread::Run, this);
}

void BackendThread::Stop() {
    if (!running_.load()) {
        return;
    }
    
    running_.store(false);
    buffer_.Stop();
    
    if (thread_.joinable()) {
        thread_.join();
    }
    
    // 处理剩余的日志
    std::vector<LogEntry> remaining;
    buffer_.SwapAndGet(remaining, 0);
    if (!remaining.empty()) {
        ProcessEntries(remaining);
    }
    
    // 刷新并关闭所有 Sink
    for (auto& sink : sinks_) {
        sink->Flush();
        sink->Close();
    }
}

void BackendThread::AddSink(SinkPtr sink) {
    sinks_.push_back(std::move(sink));
}

void BackendThread::Flush() {
    for (auto& sink : sinks_) {
        sink->Flush();
    }
}

void BackendThread::Run() {
    std::vector<LogEntry> entries;
    entries.reserve(1024);
    
    while (running_.load() || !buffer_.Empty()) {
        // 获取日志条目
        size_t count = buffer_.SwapAndGet(entries, 
            static_cast<int>(config_.flush_interval_ms));
        
        if (count > 0) {
            ProcessEntries(entries);
        }
        
        // 定期刷盘
        Flush();
    }
}

void BackendThread::ProcessEntries(const std::vector<LogEntry>& entries) {
    for (const auto& entry : entries) {
        std::string formatted = formatter_.Format(entry);
        
        for (auto& sink : sinks_) {
            // 对于 ConsoleSink，使用带颜色的输出
            auto console_sink = std::dynamic_pointer_cast<ConsoleSink>(sink);
            if (console_sink && config_.console_color) {
                console_sink->WriteWithLevel(
                    static_cast<LogLevel>(entry.level), formatted);
            } else {
                sink->Write(formatted);
            }
        }
    }
}

}  // namespace asynclog
