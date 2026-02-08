/**
 * @file file_system.cpp
 * @brief FileSystem 实现
 */

#include "file_system.h"
#include "../rpc/file_rpc_client.h"
#include "../config/config.h"

namespace swift {
namespace zone {

FileSystem::FileSystem() = default;
FileSystem::~FileSystem() = default;

bool FileSystem::Init() {
    if (!config_) return true;
    rpc_client_ = std::make_unique<FileRpcClient>();
    if (!rpc_client_->Connect(config_->file_svr_addr)) return false;
    rpc_client_->InitStub();
    return true;
}

void FileSystem::Shutdown() {
    if (rpc_client_) {
        rpc_client_->Disconnect();
        rpc_client_.reset();
    }
}

FileSystem::UploadToken FileSystem::GetUploadToken(const std::string& user_id,
                                                   const std::string& file_name,
                                                   int64_t file_size) {
    FileSystem::UploadToken out;
    if (!rpc_client_) return out;
    auto r = rpc_client_->GetUploadToken(user_id, file_name, file_size);
    if (!r.success) return out;
    out.token = r.upload_token;
    out.upload_url = r.upload_url;
    out.expire_at = r.expire_at;
    return out;
}

FileSystem::FileUrl FileSystem::GetFileUrl(const std::string& file_id,
                                           const std::string& user_id) {
    FileSystem::FileUrl out;
    if (!rpc_client_) return out;
    auto r = rpc_client_->GetFileUrl(file_id, user_id);
    if (!r.success) return out;
    out.url = r.file_url;
    out.file_name = r.file_name;
    out.file_size = r.file_size;
    out.content_type = r.content_type;
    out.expire_at = r.expire_at;
    return out;
}

bool FileSystem::DeleteFile(const std::string& file_id, const std::string& user_id) {
    if (!rpc_client_) return false;
    std::string err;
    return rpc_client_->DeleteFile(file_id, user_id, &err);
}

}  // namespace zone
}  // namespace swift
