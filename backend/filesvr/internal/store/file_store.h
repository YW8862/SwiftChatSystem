#pragma once

#include <memory>
#include <optional>
#include <string>

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
 * 上传会话（断点续传用）
 */
struct UploadSessionData {
    std::string upload_id;
    std::string user_id;
    std::string file_name;
    std::string content_type;
    int64_t file_size = 0;
    std::string md5;
    std::string msg_id;            // 可选，关联聊天消息
    std::string temp_path;         // 临时文件路径
    int64_t bytes_written = 0;
    int64_t expire_at = 0;
};

/**
 * 文件元信息存储
 *
 * RocksDB Key 设计：
 *   file:{file_id}      -> FileMetaData
 *   file_md5:{md5}      -> file_id (用于秒传检测)
 *   upload:{upload_id}  -> UploadSessionData JSON
 */
class FileStore {
public:
    virtual ~FileStore() = default;

    virtual bool Save(const FileMetaData& meta) = 0;
    virtual std::optional<FileMetaData> GetById(const std::string& file_id) = 0;
    virtual std::optional<FileMetaData> GetByMd5(const std::string& md5) = 0;
    virtual bool Delete(const std::string& file_id) = 0;

    virtual bool SaveUploadSession(const UploadSessionData& session) = 0;
    virtual std::optional<UploadSessionData> GetUploadSession(const std::string& upload_id) = 0;
    virtual bool UpdateUploadSessionBytes(const std::string& upload_id, int64_t bytes_written) = 0;
    virtual bool DeleteUploadSession(const std::string& upload_id) = 0;
};

/**
 * RocksDB 实现
 */
class RocksDBFileStore : public FileStore {
public:
    explicit RocksDBFileStore(const std::string& db_path);
    ~RocksDBFileStore() override;

    bool Save(const FileMetaData& meta) override; // 保存文件元信息
    std::optional<FileMetaData> GetById(const std::string& file_id) override; // 根据文件 ID 获取文件元信息
    std::optional<FileMetaData> GetByMd5(const std::string& md5) override; // 根据 MD5 获取文件元信息
    bool Delete(const std::string& file_id) override; // 删除文件元信息

    bool SaveUploadSession(const UploadSessionData& session) override;
    std::optional<UploadSessionData> GetUploadSession(const std::string& upload_id) override;
    bool UpdateUploadSessionBytes(const std::string& upload_id, int64_t bytes_written) override;
    bool DeleteUploadSession(const std::string& upload_id) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace swift::file
