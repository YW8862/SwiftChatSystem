/**
 * @file online_rpc_client.h
 * @brief OnlineSvr gRPC 客户端
 *
 * ZoneSvr 登录/登出直接走 OnlineSvr：Login、Logout、ValidateToken。
 */

#pragma once

#include "rpc_client_base.h"
#include "online.grpc.pb.h"
#include <memory>
#include <string>
#include <cstdint>

namespace swift {
namespace zone {

struct OnlineLoginResult {
    bool success = false;
    std::string token;
    int64_t expire_at = 0;
    std::string error;
};

struct OnlineLogoutResult {
    bool success = true;
    std::string error;
};

struct OnlineTokenResult {
    bool valid = false;
    std::string user_id;
};

class OnlineRpcClient : public RpcClientBase {
public:
    OnlineRpcClient() = default;
    ~OnlineRpcClient() override = default;

    void InitStub();

    /// 登录会话（签发 Token，单端策略）
    OnlineLoginResult Login(const std::string& user_id,
                            const std::string& device_id = "",
                            const std::string& device_type = "");

    /// 登出（移除会话）
    OnlineLogoutResult Logout(const std::string& user_id, const std::string& token);

    /// 验证 Token，返回 user_id
    OnlineTokenResult ValidateToken(const std::string& token);

private:
    std::unique_ptr<swift::online::OnlineService::Stub> stub_;
};

}  // namespace zone
}  // namespace swift
