#pragma once

#include <string>
#include <memory>
#include <vector>

namespace asynclog {

/**
 * @brief 日志输出接口（Sink 抽象基类）
 */
class Sink {
public:
    virtual ~Sink() = default;
    
    /**
     * @brief 写入单条日志
     * @param formatted_log 格式化后的日志字符串
     */
    virtual void Write(const std::string& formatted_log) = 0;
    
    /**
     * @brief 批量写入日志
     * @param logs 格式化后的日志字符串列表
     */
    virtual void WriteBatch(const std::vector<std::string>& logs) {
        for (const auto& log : logs) {
            Write(log);
        }
    }
    
    /**
     * @brief 刷新缓冲区到目标
     */
    virtual void Flush() = 0;
    
    /**
     * @brief 关闭 Sink
     */
    virtual void Close() {}
};

using SinkPtr = std::shared_ptr<Sink>;

}  // namespace asynclog
