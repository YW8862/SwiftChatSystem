/**
 * @file online_rpc_client.cpp
 * @brief OnlineSvr gRPC 客户端实现
 */

#include "online_rpc_client.h"

namespace swift {
namespace zone {

void OnlineRpcClient::InitStub() {
    stub_ = swift::online::OnlineService::NewStub(GetChannel());
}

OnlineLoginResult OnlineRpcClient::Login(const std::string& user_id,
                                         const std::string& device_id,
                                         const std::string& device_type) {
    OnlineLoginResult result;
    if (!stub_) return result;
    swift::online::LoginRequest req;
    req.set_user_id(user_id);
    req.set_device_id(device_id);
    req.set_device_type(device_type);
    swift::online::LoginResponse resp;
    auto ctx = CreateContext(5000);
    grpc::Status status = stub_->Login(ctx.get(), req, &resp);
    if (!status.ok()) {
        result.error = status.error_message();
        return result;
    }
    if (resp.code() != 0) {
        result.error = resp.message().empty() ? "login failed" : resp.message();
        return result;
    }
    result.success = true;
    result.token = resp.token();
    result.expire_at = resp.expire_at();
    return result;
}

OnlineLogoutResult OnlineRpcClient::Logout(const std::string& user_id,
                                           const std::string& token) {
    OnlineLogoutResult result;
    if (!stub_) {
        result.success = false;
        result.error = "stub not initialized";
        return result;
    }
    swift::online::LogoutRequest req;
    req.set_user_id(user_id);
    req.set_token(token);
    swift::common::CommonResponse resp;
    auto ctx = CreateContext(5000);
    grpc::Status status = stub_->Logout(ctx.get(), req, &resp);
    if (!status.ok()) {
        result.success = false;
        result.error = status.error_message();
        return result;
    }
    result.success = (resp.code() == 0);
    if (resp.code() != 0 && !resp.message().empty())
        result.error = resp.message();
    return result;
}

OnlineTokenResult OnlineRpcClient::ValidateToken(const std::string& token) {
    OnlineTokenResult result;
    if (!stub_) return result;
    swift::online::TokenRequest req;
    req.set_token(token);
    swift::online::TokenResponse resp;
    auto ctx = CreateContext(5000);
    grpc::Status status = stub_->ValidateToken(ctx.get(), req, &resp);
    if (!status.ok() || resp.code() != 0 || !resp.valid()) {
        return result;
    }
    result.valid = true;
    result.user_id = resp.user_id();
    return result;
}

}  // namespace zone
}  // namespace swift
