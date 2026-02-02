#include "async_logger/async_logger.h"
#include "ring_buffer.h"
#include "backend_thread.h"
#include "file_sink.h"
#include "console_sink.h"
#include "log_formatter.h"

#include <atomic>

namespace asynclog {

/**
 * @brief AsyncLogger 的内部实现类
 */
class AsyncLoggerImpl {
public:
    AsyncLoggerImpl() = default;
    
    ~AsyncLoggerImpl() {
        Shutdown();
    }
    
    bool Init(const LogConfig& config) {
        if (initialized_.load()) {
            return true;  // 已初始化
        }
        
        config_ = config;
        min_level_.store(static_cast<int>(config.min_level));
        
        // 创建环形缓冲区
        size_t buffer_entries = config.buffer_size / 256;  // 假设平均每条日志 256 字节
        if (buffer_entries < 1024) buffer_entries = 1024;
        buffer_ = std::make_unique<RingBuffer>(buffer_entries);
        
        // 创建后台线程
        backend_ = std::make_unique<BackendThread>(*buffer_, config);
        
        // 添加 Sink
        if (config.enable_console) {
            backend_->AddSink(std::make_shared<ConsoleSink>(config.console_color));
        }
        
        if (config.enable_file) {
            backend_->AddSink(std::make_shared<FileSink>(
                config.log_dir,
                config.file_prefix,
                config.max_file_size,
                config.max_file_count
            ));
        }
        
        // 启动后台线程
        backend_->Start();
        
        initialized_.store(true);
        return true;
    }
    
    void Shutdown() {
        if (!initialized_.load()) {
            return;
        }
        
        initialized_.store(false);
        
        if (backend_) {
            backend_->Stop();
            backend_.reset();
        }
        
        if (buffer_) {
            buffer_.reset();
        }
    }
    
    bool IsInitialized() const {
        return initialized_.load();
    }
    
    void Log(LogLevel level, const char* file, int line, const std::string& message) {
        if (!initialized_.load()) {
            return;
        }
        
        if (static_cast<int>(level) < min_level_.load()) {
            return;
        }
        
        LogEntry entry;
        entry.timestamp = LogFormatter::GetCurrentTimestamp();
        entry.level = static_cast<int>(level);
        entry.file = file ? file : "";
        entry.line = line;
        entry.message = message;
        
        buffer_->Push(std::move(entry));
    }
    
    void LogWithTag(LogLevel level, const Tag& tag,
                     const char* file, int line,
                     const std::string& message) {
        if (!initialized_.load()) {
            return;
        }
        
        if (static_cast<int>(level) < min_level_.load()) {
            return;
        }
        
        LogEntry entry;
        entry.timestamp = LogFormatter::GetCurrentTimestamp();
        entry.level = static_cast<int>(level);
        entry.file = file ? file : "";
        entry.line = line;
        entry.message = message;
        entry.tags = tag.ToString();
        
        buffer_->Push(std::move(entry));
    }
    
    void SetLevel(LogLevel level) {
        min_level_.store(static_cast<int>(level));
    }
    
    LogLevel GetLevel() const {
        return static_cast<LogLevel>(min_level_.load());
    }
    
    void Flush() {
        if (backend_) {
            buffer_->Notify();
            backend_->Flush();
        }
    }

private:
    LogConfig config_;
    std::atomic<bool> initialized_{false};
    std::atomic<int> min_level_{static_cast<int>(LogLevel::INFO)};
    
    std::unique_ptr<RingBuffer> buffer_;
    std::unique_ptr<BackendThread> backend_;
};

// ============================================================================
// AsyncLogger 单例实现
// ============================================================================

AsyncLogger& AsyncLogger::Instance() {
    static AsyncLogger instance;
    return instance;
}

AsyncLogger::AsyncLogger()
    : impl_(std::make_unique<AsyncLoggerImpl>()) {
}

AsyncLogger::~AsyncLogger() {
    Shutdown();
}

bool AsyncLogger::Init(const LogConfig& config) {
    return impl_->Init(config);
}

void AsyncLogger::Shutdown() {
    impl_->Shutdown();
}

bool AsyncLogger::IsInitialized() const {
    return impl_->IsInitialized();
}

void AsyncLogger::Log(LogLevel level, const char* file, int line, const std::string& message) {
    impl_->Log(level, file, line, message);
}

void AsyncLogger::Log(LogLevel level, const char* file, int line, 
                       const Tag& tag, const std::string& message) {
    impl_->LogWithTag(level, tag, file, line, message);
}

void AsyncLogger::SetLevel(LogLevel level) {
    impl_->SetLevel(level);
}

LogLevel AsyncLogger::GetLevel() const {
    return impl_->GetLevel();
}

void AsyncLogger::Flush() {
    impl_->Flush();
}

}  // namespace asynclog
