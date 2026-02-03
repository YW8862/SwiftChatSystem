/**
 * @file auth_system.h
 * @brief 认证子系统
 *
 * 登录/登出直接走 OnlineSvr；身份校验走 AuthSvr。
 * - 登录：AuthSvr.VerifyCredentials(username, password) → user_id + profile，
 *         再 OnlineSvr.Login(user_id, device_id, device_type) → token
 * - 登出：OnlineSvr.Logout(user_id, token)
 * - Token 校验：OnlineSvr.ValidateToken(token) → user_id
 * - 资料：AuthSvr.GetProfile / UpdateProfile
 */

#pragma once

#include "base_system.h"
#include <memory>
#include <string>
#include <cstdint>

namespace swift {
namespace zone {

class AuthRpcClient;
class OnlineRpcClient;

/// 登录结果（ZoneSvr 对外）
struct AuthLoginResult {
    bool success = false;
    std::string user_id;
    std::string token;
    int64_t expire_at = 0;
    std::string error;
    // profile 可从 AuthSvr.GetProfile 再取
};

/// 登出结果
struct AuthLogoutResult {
    bool success = true;
    std::string error;
};

/**
 * @class AuthSystem
 * @brief 认证子系统：登录/登出走 OnlineSvr，身份与资料走 AuthSvr
 */
class AuthSystem : public BaseSystem {
public:
    AuthSystem();
    ~AuthSystem() override;

    std::string Name() const override { return "AuthSystem"; }
    bool Init() override;
    void Shutdown() override;

    // ============ 业务接口 ============

    /// 登录：VerifyCredentials(AuthSvr) + Login(OnlineSvr)，返回 token
    AuthLoginResult Login(const std::string& username, const std::string& password,
                          const std::string& device_id = "",
                          const std::string& device_type = "");

    /// 登出：OnlineSvr.Logout
    AuthLogoutResult Logout(const std::string& user_id, const std::string& token);

    /// 验证 Token：OnlineSvr.ValidateToken，返回 user_id
    std::string ValidateToken(const std::string& token);

private:
    std::unique_ptr<AuthRpcClient> auth_rpc_client_;
    std::unique_ptr<OnlineRpcClient> online_rpc_client_;
};

}  // namespace zone
}  // namespace swift
