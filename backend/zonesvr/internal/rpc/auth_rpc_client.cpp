/**
 * @file auth_rpc_client.cpp
 * @brief AuthRpcClient 实现
 */

#include "auth_rpc_client.h"

namespace swift {
namespace zone {

void AuthRpcClient::InitStub() {
    stub_ = swift::auth::AuthService::NewStub(GetChannel());
}

bool AuthRpcClient::VerifyCredentials(const std::string& username,
                                      const std::string& password,
                                      std::string* out_user_id,
                                      std::string* out_error) {
    if (!stub_) return false;
    swift::auth::VerifyCredentialsRequest req;
    req.set_username(username);
    req.set_password(password);
    swift::auth::VerifyCredentialsResponse resp;
    auto ctx = CreateContext(5000);
    grpc::Status status = stub_->VerifyCredentials(ctx.get(), req, &resp);
    if (!status.ok()) {
        if (out_error) *out_error = status.error_message();
        return false;
    }
    if (resp.code() != 0) {
        if (out_error) *out_error = resp.message().empty() ? "auth failed" : resp.message();
        return false;
    }
    if (out_user_id) *out_user_id = resp.user_id();
    return true;
}

bool AuthRpcClient::GetProfile(const std::string& user_id,
                              std::string* out_nickname,
                              std::string* out_avatar_url,
                              std::string* out_error) {
    if (!stub_) return false;
    swift::auth::GetProfileRequest req;
    req.set_user_id(user_id);
    swift::auth::UserProfile resp;
    auto ctx = CreateContext(5000);
    grpc::Status status = stub_->GetProfile(ctx.get(), req, &resp);
    if (!status.ok()) {
        if (out_error) *out_error = status.error_message();
        return false;
    }
    if (out_nickname) *out_nickname = resp.nickname();
    if (out_avatar_url) *out_avatar_url = resp.avatar_url();
    return true;
}

bool AuthRpcClient::UpdateProfile(const std::string& user_id,
                                  const std::string& nickname,
                                  const std::string& avatar_url,
                                  const std::string& signature,
                                  std::string* out_error) {
    if (!stub_) return false;
    swift::auth::UpdateProfileRequest req;
    req.set_user_id(user_id);
    if (!nickname.empty()) req.set_nickname(nickname);
    if (!avatar_url.empty()) req.set_avatar_url(avatar_url);
    if (!signature.empty()) req.set_signature(signature);
    swift::common::CommonResponse resp;
    auto ctx = CreateContext(5000);
    grpc::Status status = stub_->UpdateProfile(ctx.get(), req, &resp);
    if (!status.ok()) {
        if (out_error) *out_error = status.error_message();
        return false;
    }
    if (resp.code() != 0 && out_error)
        *out_error = resp.message().empty() ? "update failed" : resp.message();
    return resp.code() == 0;
}

}  // namespace zone
}  // namespace swift
