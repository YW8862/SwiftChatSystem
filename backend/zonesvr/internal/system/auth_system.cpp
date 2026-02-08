/**
 * @file auth_system.cpp
 * @brief AuthSystem 实现：登录/登出走 OnlineSvr，身份校验走 AuthSvr
 */

#include "auth_system.h"
#include "../rpc/auth_rpc_client.h"
#include "../rpc/online_rpc_client.h"
#include "../config/config.h"

namespace swift {
namespace zone {

AuthSystem::AuthSystem() = default;
AuthSystem::~AuthSystem() = default;

bool AuthSystem::Init() {
    if (!config_) return true;  // 无配置时跳过 RPC 连接（测试等）

    auth_rpc_client_ = std::make_unique<AuthRpcClient>();
    if (!auth_rpc_client_->Connect(config_->auth_svr_addr)) return false;
    auth_rpc_client_->InitStub();

    online_rpc_client_ = std::make_unique<OnlineRpcClient>();
    if (!online_rpc_client_->Connect(config_->online_svr_addr)) return false;
    online_rpc_client_->InitStub();

    return true;
}

void AuthSystem::Shutdown() {
    if (online_rpc_client_) {
        online_rpc_client_->Disconnect();
        online_rpc_client_.reset();
    }
    if (auth_rpc_client_) {
        auth_rpc_client_->Disconnect();
        auth_rpc_client_.reset();
    }
}

AuthLoginResult AuthSystem::Login(const std::string& username,
                                   const std::string& password,
                                   const std::string& device_id,
                                   const std::string& device_type) {
    AuthLoginResult result;
    if (!auth_rpc_client_ || !online_rpc_client_) {
        result.error = "auth not configured";
        return result;
    }
    std::string user_id;
    if (!auth_rpc_client_->VerifyCredentials(username, password, &user_id, &result.error))
        return result;
    auto login = online_rpc_client_->Login(user_id, device_id, device_type);
    if (!login.success) {
        result.error = login.error;
        return result;
    }
    result.success = true;
    result.user_id = user_id;
    result.token = login.token;
    result.expire_at = login.expire_at;
    return result;
}

AuthLogoutResult AuthSystem::Logout(const std::string& user_id, const std::string& token) {
    AuthLogoutResult result;
    if (!online_rpc_client_) {
        result.success = false;
        result.error = "auth not configured";
        return result;
    }
    auto r = online_rpc_client_->Logout(user_id, token);
    result.success = r.success;
    result.error = r.error;
    return result;
}

std::string AuthSystem::ValidateToken(const std::string& token) {
    if (!online_rpc_client_) return "";
    auto r = online_rpc_client_->ValidateToken(token);
    return r.valid ? r.user_id : "";
}

}  // namespace zone
}  // namespace swift
