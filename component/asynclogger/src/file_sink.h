#pragma once

#include "sink.h"
#include <string>
#include <fstream>
#include <mutex>

namespace asynclog {

/**
 * @brief 文件输出 Sink
 * 
 * 支持：
 * - 按文件大小滚动
 * - 保留最近 N 个文件
 * - 自动创建目录
 */
class FileSink : public Sink {
public:
    /**
     * @brief 构造函数
     * @param log_dir 日志目录
     * @param file_prefix 文件名前缀
     * @param max_file_size 单文件最大大小（字节）
     * @param max_file_count 保留文件数量
     */
    FileSink(const std::string& log_dir,
             const std::string& file_prefix,
             size_t max_file_size = 100 * 1024 * 1024,
             int max_file_count = 10);
    
    ~FileSink() override;
    
    void Write(const std::string& formatted_log) override;
    void WriteBatch(const std::vector<std::string>& logs) override;
    void Flush() override;
    void Close() override;

private:
    /**
     * @brief 检查是否需要滚动文件
     */
    void CheckRotate();
    
    /**
     * @brief 滚动文件
     */
    void RotateFile();
    
    /**
     * @brief 打开新的日志文件
     */
    bool OpenNewFile();
    
    /**
     * @brief 生成当前日志文件名
     */
    std::string GenerateFileName() const;
    
    /**
     * @brief 清理旧文件
     */
    void CleanupOldFiles();
    
    /**
     * @brief 确保目录存在
     */
    bool EnsureDirectory();

private:
    std::string log_dir_;
    std::string file_prefix_;
    size_t max_file_size_;
    int max_file_count_;
    
    std::ofstream file_;
    std::string current_file_path_;
    size_t current_file_size_;
    
    std::mutex mutex_;
};

}  // namespace asynclog
