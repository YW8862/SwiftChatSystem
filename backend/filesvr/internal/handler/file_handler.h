#pragma once

#include <memory>

namespace swift::file {

class FileService;

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
 * HTTP 下载处理器
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
