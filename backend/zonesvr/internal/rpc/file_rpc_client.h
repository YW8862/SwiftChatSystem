/**
 * @file file_rpc_client.h
 * @brief FileSvr gRPC 客户端
 */

#pragma once

#include "rpc_client_base.h"
// #include "file.grpc.pb.h"

namespace swift {
namespace zone {

/**
 * @class FileRpcClient
 * @brief FileSvr 的 gRPC 客户端封装
 */
class FileRpcClient : public RpcClientBase {
public:
    FileRpcClient() = default;
    ~FileRpcClient() override = default;

    void InitStub();

    // ============ RPC 方法 ============

    /// 获取上传凭证
    // swift::file::UploadTokenResponse GetUploadToken(const swift::file::GetUploadTokenRequest& request);

    /// 获取文件URL
    // swift::file::FileUrlResponse GetFileUrl(const std::string& file_id, const std::string& user_id);

    /// 获取文件信息
    // swift::file::FileInfoResponse GetFileInfo(const std::string& file_id);

    /// 删除文件
    // swift::common::CommonResponse DeleteFile(const std::string& file_id, const std::string& user_id);

private:
    // std::unique_ptr<swift::file::FileService::Stub> stub_;
};

}  // namespace zone
}  // namespace swift
