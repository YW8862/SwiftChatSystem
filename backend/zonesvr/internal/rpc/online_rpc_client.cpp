/**
 * @file online_rpc_client.cpp
 * @brief OnlineSvr gRPC 客户端实现
 */

#include "online_rpc_client.h"
// #include "online.grpc.pb.h"

namespace swift {
namespace zone {

void OnlineRpcClient::InitStub() {
    // TODO: stub_ = swift::online::OnlineService::NewStub(GetChannel());
}

OnlineLoginResult OnlineRpcClient::Login(const std::string& user_id,
                                          const std::string& device_id,
                                          const std::string& device_type) {
    OnlineLoginResult result;
    (void)user_id;
    (void)device_id;
    (void)device_type;
    // TODO: swift::online::LoginRequest req; req.set_user_id(user_id); ...
    // auto status = stub_->Login(context.get(), req, &resp);
    return result;
}

OnlineLogoutResult OnlineRpcClient::Logout(const std::string& user_id,
                                            const std::string& token) {
    OnlineLogoutResult result;
    (void)user_id;
    (void)token;
    // TODO: stub_->Logout(...)
    return result;
}

OnlineTokenResult OnlineRpcClient::ValidateToken(const std::string& token) {
    OnlineTokenResult result;
    (void)token;
    // TODO: stub_->ValidateToken(...)
    return result;
}

}  // namespace zone
}  // namespace swift
