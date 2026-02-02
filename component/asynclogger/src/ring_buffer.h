#pragma once

#include <atomic>
#include <vector>
#include <string>
#include <mutex>
#include <condition_variable>

namespace asynclog {

/**
 * @brief 日志条目结构
 */
struct LogEntry {
    int64_t timestamp;          // 时间戳（微秒）
    int level;                  // 日志级别
    int line;                   // 行号
    std::string file;           // 文件名
    std::string message;        // 日志消息
    std::string tags;           // 格式化后的标签
    
    LogEntry() : timestamp(0), level(0), line(0) {}
};

/**
 * @brief 线程安全的环形缓冲区
 * 
 * 使用双缓冲策略：
 * - 前台缓冲区：接收业务线程的日志写入
 * - 后台缓冲区：后台线程用于刷盘
 * - 当前台缓冲区满或定时触发时，交换两个缓冲区
 */
class RingBuffer {
public:
    /**
     * @brief 构造函数
     * @param capacity 缓冲区容量（条目数）
     */
    explicit RingBuffer(size_t capacity = 8192);
    
    ~RingBuffer() = default;
    
    // 禁止拷贝
    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;
    
    /**
     * @brief 写入日志条目（生产者调用）
     * @param entry 日志条目
     * @return 是否写入成功
     */
    bool Push(LogEntry&& entry);
    
    /**
     * @brief 交换缓冲区并获取待刷盘的条目（消费者调用）
     * @param entries 输出参数，接收待刷盘的条目
     * @param timeout_ms 等待超时（毫秒），0表示不等待
     * @return 获取到的条目数量
     */
    size_t SwapAndGet(std::vector<LogEntry>& entries, int timeout_ms = 100);
    
    /**
     * @brief 通知后台线程有新数据
     */
    void Notify();
    
    /**
     * @brief 获取当前缓冲区大小
     */
    size_t Size() const;
    
    /**
     * @brief 检查缓冲区是否为空
     */
    bool Empty() const;
    
    /**
     * @brief 停止缓冲区（用于关闭时）
     */
    void Stop();
    
    /**
     * @brief 检查是否已停止
     */
    bool IsStopped() const;

private:
    size_t capacity_;
    
    // 双缓冲区
    std::vector<LogEntry> front_buffer_;   // 前台缓冲区（写入）
    std::vector<LogEntry> back_buffer_;    // 后台缓冲区（刷盘）
    
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    
    std::atomic<bool> stopped_{false};
};

}  // namespace asynclog
