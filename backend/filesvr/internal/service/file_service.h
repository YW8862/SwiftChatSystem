#pragma once

#include <memory>
#include <string>
#include <vector>
#include "../store/file_store.h"

namespace swift::file {

class FileService {
public:
    FileService(std::shared_ptr<FileStore> store, const std::string& storage_path);
    ~FileService();
    
    // 上传文件
    struct UploadResult {
        bool success;
        std::string file_id;
        std::string file_url;
        std::string thumbnail_url;
        std::string error;
    };
    UploadResult Upload(const std::string& user_id, const std::string& file_name,
                        const std::string& content_type, const std::vector<char>& data);
    
    // 获取文件 URL
    struct FileUrlResult {
        bool success;
        std::string file_url;
        std::string file_name;
        int64_t file_size;
        std::string content_type;
    };
    FileUrlResult GetFileUrl(const std::string& file_id);
    
    // 读取文件内容（用于下载）
    bool ReadFile(const std::string& file_id, std::vector<char>& data,
                  std::string& content_type, std::string& file_name);
    
    // 删除文件
    bool DeleteFile(const std::string& file_id, const std::string& user_id);
    
    // 秒传检测
    std::optional<std::string> CheckMd5(const std::string& md5);
    
private:
    std::string GenerateFileId();
    std::string BuildStoragePath(const std::string& file_id);
    std::string CalculateMd5(const std::vector<char>& data);
    
    std::shared_ptr<FileStore> store_;
    std::string storage_path_;
};

}  // namespace swift::file
