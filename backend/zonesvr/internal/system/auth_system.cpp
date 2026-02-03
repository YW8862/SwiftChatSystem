/**
 * @file auth_system.cpp
 * @brief AuthSystem 实现：登录/登出走 OnlineSvr，身份校验走 AuthSvr
 */

#include "auth_system.h"
#include "../rpc/auth_rpc_client.h"
#include "../rpc/online_rpc_client.h"

namespace swift {
namespace zone {

AuthSystem::AuthSystem() = default;
AuthSystem::~AuthSystem() = default;

bool AuthSystem::Init() {
    // TODO: auth_rpc_client_ = std::make_unique<AuthRpcClient>();
    //       auth_rpc_client_->Connect(config.auth_svr_addr);
    //       online_rpc_client_ = std::make_unique<OnlineRpcClient>();
    //       online_rpc_client_->Connect(config.online_svr_addr);
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
    // 1. AuthSvr.VerifyCredentials(username, password) → user_id, profile
    // TODO: auto verify = auth_rpc_client_->VerifyCredentials(username, password);
    // if (!verify.success) { result.error = verify.error; return result; }
    // 2. OnlineSvr.Login(user_id, device_id, device_type) → token
    // TODO: auto login = online_rpc_client_->Login(verify.user_id, device_id, device_type);
    // if (!login.success) { result.error = login.error; return result; }
    // result.success = true; result.user_id = verify.user_id; result.token = login.token;
    // result.expire_at = login.expire_at;
    (void)username;
    (void)password;
    (void)device_id;
    (void)device_type;
    return result;
}

AuthLogoutResult AuthSystem::Logout(const std::string& user_id, const std::string& token) {
    AuthLogoutResult result;
    // TODO: auto r = online_rpc_client_->Logout(user_id, token);
    // result.success = r.success; result.error = r.error;
    (void)user_id;
    (void)token;
    return result;
}

std::string AuthSystem::ValidateToken(const std::string& token) {
    // TODO: auto r = online_rpc_client_->ValidateToken(token);
    // if (r.valid) return r.user_id;
    (void)token;
    return "";
}

}  // namespace zone
}  // namespace swift
