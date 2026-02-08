/**
 * @file auth_rpc_client.h
 * @brief AuthSvr gRPC 客户端
 */

#pragma once

#include "rpc_client_base.h"
#include "auth.grpc.pb.h"
#include <memory>
#include <string>

namespace swift {
namespace zone {

/**
 * @class AuthRpcClient
 * @brief AuthSvr 的 gRPC 客户端封装：Register、VerifyCredentials、GetProfile、UpdateProfile
 */
class AuthRpcClient : public RpcClientBase {
public:
    AuthRpcClient() = default;
    ~AuthRpcClient() override = default;

    void InitStub();

    /// 校验用户名密码，返回 user_id 与 profile（登录流程第一步）
    bool VerifyCredentials(const std::string& username, const std::string& password,
                          std::string* out_user_id, std::string* out_error);

    /// 获取用户资料
    bool GetProfile(const std::string& user_id, std::string* out_nickname,
                    std::string* out_avatar_url, std::string* out_error);

    /// 更新用户资料
    bool UpdateProfile(const std::string& user_id, const std::string& nickname,
                      const std::string& avatar_url, const std::string& signature,
                      std::string* out_error);

private:
    std::unique_ptr<swift::auth::AuthService::Stub> stub_;
};

}  // namespace zone
}  // namespace swift
