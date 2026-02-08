/**
 * @file file_rpc_client.cpp
 * @brief FileRpcClient 实现
 */

#include "file_rpc_client.h"

namespace swift {
namespace zone {

void FileRpcClient::InitStub() {
    stub_ = swift::file::FileService::NewStub(GetChannel());
}

InitUploadResult FileRpcClient::InitUpload(const std::string& user_id, const std::string& file_name,
                                           const std::string& content_type, int64_t file_size,
                                           const std::string& md5, const std::string& msg_id) {
    InitUploadResult result;
    if (!stub_) return result;
    swift::file::InitUploadRequest req;
    req.set_user_id(user_id);
    req.set_file_name(file_name);
    req.set_content_type(content_type);
    req.set_file_size(file_size);
    if (!md5.empty()) req.set_md5(md5);
    if (!msg_id.empty()) req.set_msg_id(msg_id);
    swift::file::InitUploadResponse resp;
    auto ctx = CreateContext(5000);
    grpc::Status status = stub_->InitUpload(ctx.get(), req, &resp);
    if (!status.ok()) {
        result.error = status.error_message();
        return result;
    }
    if (resp.code() != 0) {
        result.error = resp.message().empty() ? "init upload failed" : resp.message();
        return result;
    }
    result.success = true;
    result.upload_id = resp.upload_id();
    result.expire_at = resp.expire_at();
    return result;
}

GetFileUrlResult FileRpcClient::GetFileUrl(const std::string& file_id, const std::string& user_id) {
    GetFileUrlResult result;
    if (!stub_) return result;
    swift::file::GetFileUrlRequest req;
    req.set_file_id(file_id);
    req.set_user_id(user_id);
    swift::file::FileUrlResponse resp;
    auto ctx = CreateContext(5000);
    grpc::Status status = stub_->GetFileUrl(ctx.get(), req, &resp);
    if (!status.ok()) {
        result.error = status.error_message();
        return result;
    }
    if (resp.code() != 0) {
        result.error = resp.message().empty() ? "get file url failed" : resp.message();
        return result;
    }
    result.success = true;
    result.file_url = resp.file_url();
    result.file_name = resp.file_name();
    result.file_size = resp.file_size();
    result.content_type = resp.content_type();
    result.expire_at = resp.expire_at();
    return result;
}

GetUploadTokenResult FileRpcClient::GetUploadToken(const std::string& user_id,
                                                    const std::string& file_name,
                                                    int64_t file_size) {
    GetUploadTokenResult result;
    if (!stub_) return result;
    swift::file::GetUploadTokenRequest req;
    req.set_user_id(user_id);
    req.set_file_name(file_name);
    req.set_file_size(file_size);
    swift::file::UploadTokenResponse resp;
    auto ctx = CreateContext(5000);
    grpc::Status status = stub_->GetUploadToken(ctx.get(), req, &resp);
    if (!status.ok()) {
        result.error = status.error_message();
        return result;
    }
    if (resp.code() != 0) {
        result.error = resp.message().empty() ? "get upload token failed" : resp.message();
        return result;
    }
    result.success = true;
    result.upload_token = resp.upload_token();
    result.upload_url = resp.upload_url();
    result.expire_at = resp.expire_at();
    return result;
}

bool FileRpcClient::DeleteFile(const std::string& file_id, const std::string& user_id,
                               std::string* out_error) {
    if (!stub_) return false;
    swift::file::DeleteFileRequest req;
    req.set_file_id(file_id);
    req.set_user_id(user_id);
    swift::file::DeleteFileResponse resp;
    auto ctx = CreateContext(5000);
    grpc::Status status = stub_->DeleteFile(ctx.get(), req, &resp);
    if (!status.ok()) {
        if (out_error) *out_error = status.error_message();
        return false;
    }
    if (resp.code() != 0 && out_error)
        *out_error = resp.message().empty() ? "delete file failed" : resp.message();
    return resp.code() == 0;
}

}  // namespace zone
}  // namespace swift
