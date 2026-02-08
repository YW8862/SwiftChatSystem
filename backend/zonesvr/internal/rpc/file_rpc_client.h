/**
 * @file file_rpc_client.h
 * @brief FileSvr gRPC 客户端
 */

#pragma once

#include "rpc_client_base.h"
#include "file.grpc.pb.h"
#include <memory>
#include <string>
#include <cstdint>

namespace swift {
namespace zone {

struct InitUploadResult {
    bool success = false;
    std::string upload_id;
    int64_t expire_at = 0;
    std::string error;
};

struct GetFileUrlResult {
    bool success = false;
    std::string file_url;
    std::string file_name;
    int64_t file_size = 0;
    std::string content_type;
    int64_t expire_at = 0;
    std::string error;
};

struct GetUploadTokenResult {
    bool success = false;
    std::string upload_token;
    std::string upload_url;
    int64_t expire_at = 0;
    std::string error;
};

/**
 * @class FileRpcClient
 * @brief FileSvr 的 gRPC 客户端封装
 */
class FileRpcClient : public RpcClientBase {
public:
    FileRpcClient() = default;
    ~FileRpcClient() override = default;

    void InitStub();

    InitUploadResult InitUpload(const std::string& user_id, const std::string& file_name,
                                const std::string& content_type, int64_t file_size,
                                const std::string& md5, const std::string& msg_id);
    GetFileUrlResult GetFileUrl(const std::string& file_id, const std::string& user_id);
    GetUploadTokenResult GetUploadToken(const std::string& user_id, const std::string& file_name,
                                        int64_t file_size);
    bool DeleteFile(const std::string& file_id, const std::string& user_id, std::string* out_error);

private:
    std::unique_ptr<swift::file::FileService::Stub> stub_;
};

}  // namespace zone
}  // namespace swift
