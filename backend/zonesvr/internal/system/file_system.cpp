/**
 * @file file_system.cpp
 * @brief FileSystem 实现
 */

#include "file_system.h"
#include "../rpc/file_rpc_client.h"

namespace swift {
namespace zone {

FileSystem::FileSystem() = default;
FileSystem::~FileSystem() = default;

bool FileSystem::Init() {
    // TODO: 创建 RPC Client，连接 FileSvr
    return true;
}

void FileSystem::Shutdown() {
    if (rpc_client_) {
        rpc_client_.reset();
    }
}

FileSystem::UploadToken FileSystem::GetUploadToken(const std::string& user_id,
                                                    const std::string& file_name,
                                                    int64_t file_size) {
    // TODO: 调用 FileSvr.GetUploadToken
    return {};
}

FileSystem::FileUrl FileSystem::GetFileUrl(const std::string& file_id, 
                                            const std::string& user_id) {
    // TODO: 调用 FileSvr.GetFileUrl
    return {};
}

bool FileSystem::DeleteFile(const std::string& file_id, const std::string& user_id) {
    // TODO: 调用 FileSvr.DeleteFile
    return false;
}

}  // namespace zone
}  // namespace swift
