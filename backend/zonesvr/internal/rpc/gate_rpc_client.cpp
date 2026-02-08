/**
 * @file gate_rpc_client.cpp
 * @brief GateRpcClient 实现
 */

#include "gate_rpc_client.h"

namespace swift {
namespace zone {

void GateRpcClient::InitStub() {
    stub_ = swift::gate::GateInternalService::NewStub(GetChannel());
}

/**
 * @brief 推送消息给用户
 * @param user_id 用户ID
 * @param cmd 命令
 * @param payload 负载
 * @param out_error 错误信息
 * @return 是否成功
 */
bool GateRpcClient::PushMessage(const std::string& user_id, const std::string& cmd,
                                const std::string& payload, std::string* out_error) {
    if (!stub_) return false;
    swift::gate::PushMessageRequest req;
    req.set_user_id(user_id);
    req.set_cmd(cmd);
    req.set_payload(payload);
    swift::common::CommonResponse resp;
    auto ctx = CreateContext(5000);
    grpc::Status status = stub_->PushMessage(ctx.get(), req, &resp);
    if (!status.ok()) {
        if (out_error) *out_error = status.error_message();
        return false;
    }
    if (resp.code() != 0 && out_error)
        *out_error = resp.message().empty() ? "push failed" : resp.message();
    return resp.code() == 0;
}

bool GateRpcClient::DisconnectUser(const std::string& user_id, const std::string& reason,
                                   std::string* out_error) {
    if (!stub_) return false;
    swift::gate::DisconnectUserRequest req;
    req.set_user_id(user_id);
    req.set_reason(reason);
    swift::common::CommonResponse resp;
    auto ctx = CreateContext(5000);
    grpc::Status status = stub_->DisconnectUser(ctx.get(), req, &resp);
    if (!status.ok()) {
        if (out_error) *out_error = status.error_message();
        return false;
    }
    if (resp.code() != 0 && out_error)
        *out_error = resp.message().empty() ? "disconnect failed" : resp.message();
    return resp.code() == 0;
}

}  // namespace zone
}  // namespace swift
