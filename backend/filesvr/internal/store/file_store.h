#pragma once

#include <string>
#include <optional>

namespace swift::file {

/**
 * 文件元信息
 */
struct FileMetaData {
    std::string file_id;
    std::string file_name;
    std::string content_type;
    int64_t file_size = 0;
    std::string md5;
    std::string uploader_id;
    std::string storage_path;    // 本地存储路径
    int64_t uploaded_at = 0;
};

/**
 * 文件元信息存储
 * 
 * RocksDB Key 设计：
 *   file:{file_id}    -> FileMetaData
 *   file_md5:{md5}    -> file_id (用于秒传检测)
 */
class FileStore {
public:
    virtual ~FileStore() = default;
    
    virtual bool Save(const FileMetaData& meta) = 0;
    virtual std::optional<FileMetaData> GetById(const std::string& file_id) = 0;
    virtual std::optional<FileMetaData> GetByMd5(const std::string& md5) = 0;
    virtual bool Delete(const std::string& file_id) = 0;
};

}  // namespace swift::file
