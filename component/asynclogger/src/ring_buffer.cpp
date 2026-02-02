#include "ring_buffer.h"
#include <chrono>

namespace asynclog {

RingBuffer::RingBuffer(size_t capacity)
    : capacity_(capacity) {
    front_buffer_.reserve(capacity);
    back_buffer_.reserve(capacity);
}

bool RingBuffer::Push(LogEntry&& entry) {
    if (stopped_.load(std::memory_order_relaxed)) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 如果前台缓冲区满了，丢弃最老的日志（或者可以选择阻塞）
    if (front_buffer_.size() >= capacity_) {
        // 策略：丢弃，避免阻塞业务线程
        return false;
    }
    
    front_buffer_.push_back(std::move(entry));
    
    // 如果缓冲区达到一定阈值，通知后台线程
    if (front_buffer_.size() >= capacity_ / 2) {
        cv_.notify_one();
    }
    
    return true;
}

size_t RingBuffer::SwapAndGet(std::vector<LogEntry>& entries, int timeout_ms) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // 等待数据或超时
    if (front_buffer_.empty() && !stopped_.load()) {
        if (timeout_ms > 0) {
            cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this] {
                return !front_buffer_.empty() || stopped_.load();
            });
        }
    }
    
    // 交换缓冲区
    if (!front_buffer_.empty()) {
        back_buffer_.clear();
        std::swap(front_buffer_, back_buffer_);
        entries = std::move(back_buffer_);
        back_buffer_.clear();
        return entries.size();
    }
    
    entries.clear();
    return 0;
}

void RingBuffer::Notify() {
    cv_.notify_one();
}

size_t RingBuffer::Size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return front_buffer_.size();
}

bool RingBuffer::Empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return front_buffer_.empty();
}

void RingBuffer::Stop() {
    stopped_.store(true, std::memory_order_release);
    cv_.notify_all();
}

bool RingBuffer::IsStopped() const {
    return stopped_.load(std::memory_order_acquire);
}

}  // namespace asynclog
