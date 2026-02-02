/**
 * @file chat_system.h
 * @brief 聊天子系统 - RPC 转发层
 * 
 * ChatSystem 持有 ChatSvr 的 RPC Client，负责将请求转发到 ChatSvr。
 * 实际的业务逻辑（消息存储、离线队列等）在 ChatSvr 中实现。
 * 
 * 职责：
 * - 封装 ChatSvr 的 RPC 接口
 * - 转发消息相关请求
 * - 协调 SessionStore 进行消息路由（推送给在线用户）
 */

#pragma once

#include "base_system.h"
#include <memory>
#include <string>

namespace swift {
namespace zone {

// 前向声明
class ChatRpcClient;
class GateRpcClient;

/**
 * @class ChatSystem
 * @brief 聊天子系统 - ChatSvr 的 RPC 转发层
 */
class ChatSystem : public BaseSystem {
public:
    ChatSystem();
    ~ChatSystem() override;

    std::string Name() const override { return "ChatSystem"; }
    bool Init() override;
    void Shutdown() override;

    // ============ RPC 转发接口 ============
    // 这些接口将请求转发到 ChatSvr，实际逻辑在 ChatSvr 实现

    /// 发送消息 → ChatSvr.SendMessage
    // SendMessageResponse SendMessage(const SendMessageRequest& request);

    /// 撤回消息 → ChatSvr.RecallMessage
    // CommonResponse RecallMessage(const RecallMessageRequest& request);

    /// 拉取离线消息 → ChatSvr.PullOffline
    // PullOfflineResponse PullOffline(const PullOfflineRequest& request);

    /// 获取历史消息 → ChatSvr.GetHistory
    // GetHistoryResponse GetHistory(const GetHistoryRequest& request);

    /// 标记已读 → ChatSvr.MarkRead
    // CommonResponse MarkRead(const MarkReadRequest& request);

    // ============ 消息路由（ZoneSvr 特有职责）============

    /// 将消息推送给在线用户（查 SessionStore，调用 Gate.PushMessage）
    bool PushToUser(const std::string& user_id, 
                    const std::string& cmd, 
                    const std::string& payload);

private:
    std::unique_ptr<ChatRpcClient> rpc_client_;  // ChatSvr Client
};

}  // namespace zone
}  // namespace swift
