/**
 * @file file_system.h
 * @brief 文件子系统
 * 
 * FileSystem 负责处理文件相关的请求：
 * - 获取上传凭证
 * - 获取文件下载URL
 * - 获取文件信息
 * 
 * 内部调用 FileSvr 的 gRPC 接口完成实际业务。
 * 
 * 注意：实际的文件上传/下载通过 HTTP 直接与 FileSvr 交互，
 * 此处只处理凭证和元信息相关的请求。
 */

#pragma once

#include "base_system.h"
#include <memory>
#include <string>

namespace swift {
namespace zone {

// 前向声明
class FileRpcClient;

/**
 * @class FileSystem
 * @brief 文件子系统
 */
class FileSystem : public BaseSystem {
public:
    FileSystem();
    ~FileSystem() override;

    std::string Name() const override { return "FileSystem"; }
    bool Init() override;
    void Shutdown() override;

    // ============ 业务接口 ============

    /// 获取上传凭证
    /// @return 上传Token和URL
    struct UploadToken {
        std::string token;
        std::string upload_url;
        int64_t expire_at;
    };
    UploadToken GetUploadToken(const std::string& user_id,
                               const std::string& file_name,
                               int64_t file_size);

    /// 获取文件下载URL
    struct FileUrl {
        std::string url;
        std::string file_name;
        int64_t file_size;
        std::string content_type;
        int64_t expire_at;
    };
    FileUrl GetFileUrl(const std::string& file_id, const std::string& user_id);

    /// 获取文件信息
    // FileInfo GetFileInfo(const std::string& file_id);

    /// 删除文件
    bool DeleteFile(const std::string& file_id, const std::string& user_id);

private:
    std::unique_ptr<FileRpcClient> rpc_client_;
};

}  // namespace zone
}  // namespace swift
