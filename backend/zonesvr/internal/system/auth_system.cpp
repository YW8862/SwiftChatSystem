/**
 * @file auth_system.cpp
 * @brief AuthSystem 实现
 */

#include "auth_system.h"
#include "../rpc/auth_rpc_client.h"

namespace swift {
namespace zone {

AuthSystem::AuthSystem() = default;
AuthSystem::~AuthSystem() = default;

bool AuthSystem::Init() {
    // TODO: 创建 RPC Client，连接 AuthSvr
    // rpc_client_ = std::make_unique<AuthRpcClient>();
    // return rpc_client_->Connect(config.auth_svr_addr);
    return true;
}

void AuthSystem::Shutdown() {
    if (rpc_client_) {
        // rpc_client_->Disconnect();
        rpc_client_.reset();
    }
}

std::string AuthSystem::ValidateToken(const std::string& token) {
    // TODO: 调用 AuthSvr.ValidateToken RPC
    // auto response = rpc_client_->ValidateToken(token);
    // if (response.valid) {
    //     return response.user_id;
    // }
    return "";
}

}  // namespace zone
}  // namespace swift
