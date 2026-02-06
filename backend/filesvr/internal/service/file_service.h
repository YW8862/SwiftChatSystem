#pragma once

#include <memory>
#include <string>
#include <vector>
#include "../config/config.h"
#include "../store/file_store.h"
#include "swift/error_code.h"

namespace swift::file {

/**
 * 业务逻辑层，与 proto 生成的 FileService 区分。
 */
class FileServiceCore {
public:
    FileServiceCore(std::shared_ptr<FileStore> store, const FileConfig& config);
    ~FileServiceCore();

    // ----- 上传会话（断点续传） -----
    struct InitUploadResult {
        bool success = false;
        int error_code = 0;        // swift::ErrorCode 数值，失败时由 error_code.h 定义
        std::string error;         // 失败时与 error_code 对应的描述（ErrorCodeToString）
        std::string upload_id;
        int64_t expire_at = 0;
        std::string existing_file_id;  // 秒传时已有文件 ID
    };

    /**
     * 初始化上传会话
     * @param user_id 用户 ID
     * @param file_name 文件名
     * @param content_type 文件类型
     * @param file_size 文件大小
     * @param md5 文件 MD5
     * @param msg_id 消息 ID
     */
    InitUploadResult InitUpload(const std::string& user_id, const std::string& file_name,
                                const std::string& content_type, int64_t file_size,
                                const std::string& md5, const std::string& msg_id);

    struct GetUploadStateResult {
        bool found = false;
        int error_code = 0;
        std::string error;
        int64_t offset = 0;
        int64_t file_size = 0;
        bool completed = false;
        std::string file_id;
        int64_t expire_at = 0;
    };

    /**
     * 获取上传状态
     * @param upload_id 上传会话 ID
     */
    GetUploadStateResult GetUploadState(const std::string& upload_id);

    struct AppendChunkResult {
        bool success = false;
        int error_code = 0;
        std::string error;
        int64_t new_offset = 0;
    };

    /**
     * 追加文件块
     * @param upload_id 上传会话 ID
     * @param data 文件数据
     * @param size 文件数据大小
     */
    AppendChunkResult AppendChunk(const std::string& upload_id, const void* data, size_t size);

    struct CompleteUploadResult {
        bool success = false;
        int error_code = 0;
        std::string error;
        std::string file_id;
        std::string file_url;
        std::string thumbnail_url;  // 可选，暂空
    };

    /**
     * 完成上传
     * @param upload_id 上传会话 ID
     */
    CompleteUploadResult CompleteUpload(const std::string& upload_id);

    // ----- 一次性上传（小文件或兼容） -----
    struct UploadResult {
        bool success = false;
        int error_code = 0;
        std::string file_id;
        std::string file_url;
        std::string thumbnail_url;
        std::string error;
    };

    /**
     * 一次性上传
     * @param user_id 用户 ID
     * @param file_name 文件名
     * @param content_type 文件类型
     * @param data 文件数据
     */
    UploadResult Upload(const std::string& user_id, const std::string& file_name,
                        const std::string& content_type, const std::vector<char>& data);

    // ----- 下载与元信息 -----
    struct FileUrlResult {
        bool success = false;
        int error_code = 0;
        std::string error;
        std::string file_url;
        std::string file_name;
        int64_t file_size = 0;
        std::string content_type;
        int64_t expire_at = 0;  // URL 过期时间戳，可选
    };

    /**
     * 获取文件 URL
     * @param file_id 文件 ID
     * @param user_id 用户 ID
     */
    FileUrlResult GetFileUrl(const std::string& file_id, const std::string& user_id);

    /**
     * 读取文件
     * @param file_id 文件 ID
     * @param data 文件数据
     * @param content_type 文件类型
     * @param file_name 文件名
     */
    bool ReadFile(const std::string& file_id, std::vector<char>& data,
                  std::string& content_type, std::string& file_name);

    /**
     * 按范围读取文件（用于 HTTP Range 断点续传）
     * @param file_id 文件 ID
     * @param offset 起始字节
     * @param length 读取长度（-1 表示到文件末尾）
     * @param data 输出数据
     * @param content_type 输出 MIME 类型
     * @param file_name 输出文件名
     * @param file_size 输出文件总大小
     * @return 是否成功
     */
    bool ReadFileRange(const std::string& file_id, int64_t offset, int64_t length,
                       std::vector<char>& data, std::string& content_type, std::string& file_name,
                       int64_t& file_size);

    struct FileInfoResult {
        bool found = false;
        FileMetaData meta;
    };

    /**
     * 获取文件信息
     * @param file_id 文件 ID
     */
    FileInfoResult GetFileInfo(const std::string& file_id);

    /**
     * 删除文件
     * @param file_id 文件 ID
     * @param user_id 用户 ID
     */
    bool DeleteFile(const std::string& file_id, const std::string& user_id); 

    /**
     * 检查 MD5
     * @param md5 文件 MD5
     */
    std::optional<std::string> CheckMd5(const std::string& md5); 

    struct UploadTokenResult {
        bool success = false;
        int error_code = 0;
        std::string error;
        std::string upload_token;
        std::string upload_url;
        int64_t expire_at = 0;
    };

    /**
     * 获取上传凭证
     * @param user_id 用户 ID
     * @param file_name 文件名
     * @param file_size 文件大小
     */
    UploadTokenResult GetUploadToken(const std::string& user_id, const std::string& file_name,
                                     int64_t file_size);

private:
    std::string GenerateFileId(); // 生成文件 ID
    std::string BuildStoragePath(const std::string& file_id); // 构建存储路径
    std::string BuildFileUrl(const std::string& file_id); // 构建文件 URL
    std::string GetTempPath(const std::string& upload_id); // 获取临时路径
    int64_t NowSeconds() const; // 获取当前时间戳

    std::shared_ptr<FileStore> store_;
    FileConfig config_;
};

}  // namespace swift::file
