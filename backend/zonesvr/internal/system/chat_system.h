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
#include "../rpc/chat_rpc_client.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>

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

    /// 发送消息 → ChatSvr.SendMessage
    struct SendMessageResult {
        bool success = false;
        std::string msg_id;
        int64_t timestamp = 0;
        std::string error;
    };
    SendMessageResult SendMessage(const std::string& from_user_id, const std::string& to_id,
                                  int32_t chat_type, const std::string& content,
                                  const std::string& media_url = "", const std::string& media_type = "",
                                  const std::vector<std::string>& mentions = {},
                                  const std::string& reply_to_msg_id = "",
                                  const std::string& client_msg_id = "", int64_t file_size = 0);

    /// 撤回消息 → ChatSvr.RecallMessage
    bool RecallMessage(const std::string& msg_id, const std::string& user_id, std::string* out_error);

    /// 拉取离线消息 → ChatSvr.PullOffline
    struct OfflineMessage {
        std::string msg_id;
        std::string from_user_id;
        std::string to_id;
        int32_t chat_type = 0;
        std::string content;
        std::string media_url;
        std::string media_type;
        int64_t timestamp = 0;
    };
    struct OfflineResult {
        bool success = false;
        std::vector<OfflineMessage> messages;
        std::string next_cursor;
        bool has_more = false;
        std::string error;
    };
    OfflineResult PullOffline(const std::string& user_id, int32_t limit, const std::string& cursor);

    /// 标记已读 → ChatSvr.MarkRead
    bool MarkRead(const std::string& user_id, const std::string& chat_id, int32_t chat_type,
                  const std::string& last_msg_id, std::string* out_error);

    /// 会话历史 → ChatSvr.GetHistory
    struct GetHistoryResult {
        bool success = false;
        std::vector<ChatMessageResult> messages;
        bool has_more = false;
        std::string error;
    };
    GetHistoryResult GetHistory(const std::string& user_id, const std::string& chat_id,
                                int32_t chat_type, const std::string& before_msg_id, int32_t limit);

    /// 同步会话列表 → ChatSvr.SyncConversations
    struct SyncConversationsResult {
        bool success = false;
        std::vector<ConversationResult> conversations;
        std::string error;
    };
    SyncConversationsResult SyncConversations(const std::string& user_id, int64_t last_sync_time);

    /// 删除会话 → ChatSvr.DeleteConversation
    bool DeleteConversation(const std::string& user_id, const std::string& chat_id,
                            int32_t chat_type, std::string* out_error);

    // ============ 消息路由（ZoneSvr 特有职责）============

    /// 注入由 Zone 层提供的“推送到用户”回调（RouteToUser），用于 PushToUser 实际投递
    using PushToUserCallback = std::function<bool(const std::string& user_id,
                                                   const std::string& cmd,
                                                   const std::string& payload)>;
    void SetPushToUserCallback(PushToUserCallback cb) { push_to_user_callback_ = std::move(cb); }

    /// 将消息推送给在线用户；依赖 SetPushToUserCallback 注入的回调（主推送链路在 Zone 的 RouteToUser）
    bool PushToUser(const std::string& user_id, const std::string& cmd, const std::string& payload);

private:
    std::unique_ptr<ChatRpcClient> rpc_client_;  // ChatSvr Client
    PushToUserCallback push_to_user_callback_;
};

}  // namespace zone
}  // namespace swift
