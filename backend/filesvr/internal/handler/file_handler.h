#pragma once

#include <memory>
#include "file.grpc.pb.h"

namespace swift::file {

class FileServiceCore;

/**
 * 对外 API 层（Handler）
 * 直接实现 proto 定义的 FileService gRPC 接口，无独立 API 层。
 */
class FileHandler : public ::swift::file::FileService::Service {
public:
    /**
     * @param service 业务逻辑层
     * @param jwt_secret 可选，与 OnlineSvr 一致时从 metadata 校验 Token 得到当前 user_id；空则使用请求体中的 user_id
     */
    FileHandler(std::shared_ptr<FileServiceCore> service, const std::string& jwt_secret = "");
    ~FileHandler() override;

    ::grpc::Status InitUpload(::grpc::ServerContext* context,
                              const ::swift::file::InitUploadRequest* request,
                              ::swift::file::InitUploadResponse* response) override;

    ::grpc::Status GetUploadState(::grpc::ServerContext* context,
                                  const ::swift::file::GetUploadStateRequest* request,
                                  ::swift::file::GetUploadStateResponse* response) override;

    ::grpc::Status UploadFile(::grpc::ServerContext* context,
                              ::grpc::ServerReader<::swift::file::UploadChunk>* reader,
                              ::swift::file::UploadResponse* response) override;

    ::grpc::Status GetFileUrl(::grpc::ServerContext* context,
                              const ::swift::file::GetFileUrlRequest* request,
                              ::swift::file::FileUrlResponse* response) override;

    ::grpc::Status GetFileInfo(::grpc::ServerContext* context,
                               const ::swift::file::GetFileInfoRequest* request,
                               ::swift::file::FileInfoResponse* response) override;

    ::grpc::Status DeleteFile(::grpc::ServerContext* context,
                              const ::swift::file::DeleteFileRequest* request,
                              ::swift::file::DeleteFileResponse* response) override;

    ::grpc::Status GetUploadToken(::grpc::ServerContext* context,
                                  const ::swift::file::GetUploadTokenRequest* request,
                                  ::swift::file::UploadTokenResponse* response) override;

private:
    std::string GetUserId(::grpc::ServerContext* context, const std::string& request_user_id) const;

    std::shared_ptr<FileServiceCore> service_;
    std::string jwt_secret_;
};

/**
 * HTTP 下载对外接口（Handler）
 * GET /files/{file_id} 由 HTTP 层调用，支持 Range 断点续传。
 */
class HttpDownloadHandler {
public:
    explicit HttpDownloadHandler(std::shared_ptr<FileServiceCore> service);
    ~HttpDownloadHandler();

    /**
     * 获取文件或范围内容，用于 HTTP 响应。
     * @param file_id 文件 ID
     * @param range_start 范围起始（-1 表示从头）
     * @param range_end 范围结束（-1 表示到末尾）
     * @param data 输出内容
     * @param content_type 输出 MIME
     * @param file_name 输出文件名
     * @param total_size 输出文件总大小
     * @param is_partial 是否为部分内容（206）
     * @return 是否找到并读取成功
     */
    bool Serve(const std::string& file_id, int64_t range_start, int64_t range_end,
               std::vector<char>& data, std::string& content_type, std::string& file_name,
               int64_t& total_size, bool& is_partial);

private:
    std::shared_ptr<FileServiceCore> service_;
};

}  // namespace swift::file
