/**
 * @file chat_rpc_client.h
 * @brief ChatSvr gRPC 客户端
 */

#pragma once

#include "rpc_client_base.h"
// #include "chat.grpc.pb.h"

namespace swift {
namespace zone {

/**
 * @class ChatRpcClient
 * @brief ChatSvr 的 gRPC 客户端封装
 */
class ChatRpcClient : public RpcClientBase {
public:
    ChatRpcClient() = default;
    ~ChatRpcClient() override = default;

    void InitStub();

    // ============ RPC 方法 ============

    /// 发送消息
    // swift::chat::SendMessageResponse SendMessage(const swift::chat::SendMessageRequest& request);

    /// 撤回消息
    // swift::common::CommonResponse RecallMessage(const std::string& msg_id, const std::string& user_id);

    /// 拉取离线消息
    // swift::chat::PullOfflineResponse PullOffline(const swift::chat::PullOfflineRequest& request);

    /// 获取历史消息
    // swift::chat::GetHistoryResponse GetHistory(const swift::chat::GetHistoryRequest& request);

    /// 标记已读
    // swift::common::CommonResponse MarkRead(const swift::chat::MarkReadRequest& request);

private:
    // std::unique_ptr<swift::chat::ChatService::Stub> stub_;
};

}  // namespace zone
}  // namespace swift
