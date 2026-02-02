/**
 * @file auth_rpc_client.h
 * @brief AuthSvr gRPC 客户端
 */

#pragma once

#include "rpc_client_base.h"
// #include "auth.grpc.pb.h"

namespace swift {
namespace zone {

/**
 * @class AuthRpcClient
 * @brief AuthSvr 的 gRPC 客户端封装
 */
class AuthRpcClient : public RpcClientBase {
public:
    AuthRpcClient() = default;
    ~AuthRpcClient() override = default;

    /// 初始化 Stub
    void InitStub();

    // ============ RPC 方法 ============

    /// 验证 Token
    // swift::auth::TokenResponse ValidateToken(const std::string& token);

    /// 获取用户资料
    // swift::auth::UserProfile GetProfile(const std::string& user_id);

    /// 更新用户资料
    // swift::common::CommonResponse UpdateProfile(const swift::auth::UpdateProfileRequest& request);

private:
    // std::unique_ptr<swift::auth::AuthService::Stub> stub_;
};

}  // namespace zone
}  // namespace swift
