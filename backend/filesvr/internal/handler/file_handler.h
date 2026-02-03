#pragma once

#include <memory>

namespace swift::file {

class FileService;

/**
 * 对外 API 层（Handler）
 * 直接实现 proto 定义的 FileService gRPC 接口，无独立 API 层。
 */
class FileHandler {
public:
    explicit FileHandler(std::shared_ptr<FileService> service);
    ~FileHandler();
    
    // gRPC 接口
    // UploadFile (streaming)
    // GetFileUrl
    // GetFileInfo
    // DeleteFile
    
private:
    std::shared_ptr<FileService> service_;
};

/**
 * HTTP 下载对外接口（Handler）
 * GET /files/{file_id} 直接对外提供
 */
class HttpDownloadHandler {
public:
    explicit HttpDownloadHandler(std::shared_ptr<FileService> service);
    ~HttpDownloadHandler();
    
    // GET /files/{file_id}
    // 返回文件内容
    
private:
    std::shared_ptr<FileService> service_;
};

}  // namespace swift::file
