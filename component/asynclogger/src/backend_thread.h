#pragma once

#include "ring_buffer.h"
#include "sink.h"
#include "log_formatter.h"
#include "../include/async_logger/log_config.h"

#include <thread>
#include <vector>
#include <memory>
#include <atomic>

namespace asynclog {

/**
 * @brief 后台刷盘线程
 * 
 * 负责：
 * - 定期从 RingBuffer 获取日志
 * - 格式化日志
 * - 写入各个 Sink
 */
class BackendThread {
public:
    /**
     * @brief 构造函数
     * @param buffer 环形缓冲区
     * @param config 日志配置
     */
    BackendThread(RingBuffer& buffer, const LogConfig& config);
    
    ~BackendThread();
    
    /**
     * @brief 启动后台线程
     */
    void Start();
    
    /**
     * @brief 停止后台线程
     */
    void Stop();
    
    /**
     * @brief 添加 Sink
     */
    void AddSink(SinkPtr sink);
    
    /**
     * @brief 立即刷新所有 Sink
     */
    void Flush();

private:
    /**
     * @brief 后台线程主循环
     */
    void Run();
    
    /**
     * @brief 处理日志条目
     */
    void ProcessEntries(const std::vector<LogEntry>& entries);

private:
    RingBuffer& buffer_;
    LogConfig config_;
    LogFormatter formatter_;
    
    std::vector<SinkPtr> sinks_;
    std::thread thread_;
    std::atomic<bool> running_{false};
};

}  // namespace asynclog
