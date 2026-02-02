/**
 * @file auth_rpc_client.cpp
 * @brief AuthRpcClient 实现
 */

#include "auth_rpc_client.h"

namespace swift {
namespace zone {

void AuthRpcClient::InitStub() {
    // TODO: stub_ = swift::auth::AuthService::NewStub(GetChannel());
}

// swift::auth::TokenResponse AuthRpcClient::ValidateToken(const std::string& token) {
//     swift::auth::TokenRequest request;
//     request.set_token(token);
//     
//     swift::auth::TokenResponse response;
//     auto context = CreateContext();
//     
//     auto status = stub_->ValidateToken(context.get(), request, &response);
//     if (!status.ok()) {
//         // LOG_ERROR("ValidateToken failed: {}", status.error_message());
//     }
//     return response;
// }

}  // namespace zone
}  // namespace swift
