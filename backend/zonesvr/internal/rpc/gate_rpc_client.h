/**
 * @file gate_rpc_client.h
 * @brief GateSvr gRPC 客户端
 * 
 * 用于 ZoneSvr 向 GateSvr 推送消息、断开用户连接等。
 */

#pragma once

#include "rpc_client_base.h"
// #include "gate.grpc.pb.h"

namespace swift {
namespace zone {

/**
 * @class GateRpcClient
 * @brief GateSvr 的 gRPC 客户端封装
 * 
 * ZoneSvr 需要维护多个 GateRpcClient（每个 Gate 一个）
 */
class GateRpcClient : public RpcClientBase {
public:
    GateRpcClient() = default;
    ~GateRpcClient() override = default;

    void InitStub();

    // ============ RPC 方法 ============

    /// 推送消息给用户
    // swift::common::CommonResponse PushMessage(const std::string& user_id, 
    //                                           const std::string& cmd,
    //                                           const std::string& payload);

    /// 断开用户连接
    // swift::common::CommonResponse DisconnectUser(const std::string& user_id, const std::string& reason);

private:
    // std::unique_ptr<swift::gate::GateInternalService::Stub> stub_;
};

}  // namespace zone
}  // namespace swift
