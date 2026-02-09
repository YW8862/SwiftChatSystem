/**
 * @file chat_system.cpp
 * @brief ChatSystem 实现 - RPC 转发层
 */

#include "chat_system.h"
#include "../rpc/chat_rpc_client.h"
#include "../rpc/gate_rpc_client.h"
#include "../store/session_store.h"
#include "../config/config.h"

namespace swift {
namespace zone {

ChatSystem::ChatSystem() = default;
ChatSystem::~ChatSystem() = default;

bool ChatSystem::Init() {
    if (!config_) return true;
    rpc_client_ = std::make_unique<ChatRpcClient>();
    if (!rpc_client_->Connect(config_->chat_svr_addr, !config_->standalone)) return false;
    rpc_client_->InitStub();
    return true;
}

void ChatSystem::Shutdown() {
    if (rpc_client_) {
        rpc_client_->Disconnect();
        rpc_client_.reset();
    }
}

ChatSystem::SendMessageResult ChatSystem::SendMessage(const std::string& from_user_id,
                                                      const std::string& to_id,
                                                      int32_t chat_type,
                                                      const std::string& content,
                                                      const std::string& media_url,
                                                      const std::string& media_type,
                                                      const std::vector<std::string>& mentions,
                                                      const std::string& reply_to_msg_id,
                                                      const std::string& client_msg_id,
                                                      int64_t file_size) {
    SendMessageResult result;
    if (!rpc_client_) return result;
    auto r = rpc_client_->SendMessage(from_user_id, to_id, chat_type, content,
                                      media_url, media_type, mentions, reply_to_msg_id,
                                      client_msg_id, file_size);
    result.success = r.success;
    result.msg_id = r.msg_id;
    result.timestamp = r.timestamp;
    result.error = r.error;
    return result;
}

ChatSystem::OfflineResult ChatSystem::PullOffline(const std::string& user_id,
                                                  int32_t limit,
                                                  const std::string& cursor) {
    OfflineResult result;
    if (!rpc_client_) {
        result.error = "ChatSystem not available";
        return result;
    }
    std::vector<ChatMessageResult> tmp;
    std::string next;
    bool has_more = false;
    std::string err;
    if (!rpc_client_->PullOffline(user_id, limit, cursor, &tmp, &next, &has_more, &err)) {
        result.error = err;
        return result;
    }
    result.success = true;
    result.next_cursor = next;
    result.has_more = has_more;
    result.messages.reserve(tmp.size());
    for (const auto& m : tmp) {
        OfflineMessage om;
        om.msg_id = m.msg_id;
        om.from_user_id = m.from_user_id;
        om.to_id = m.to_id;
        om.chat_type = m.chat_type;
        om.content = m.content;
        om.media_url = m.media_url;
        om.media_type = m.media_type;
        om.timestamp = m.timestamp;
        result.messages.push_back(std::move(om));
    }
    return result;
}

bool ChatSystem::RecallMessage(const std::string& msg_id, const std::string& user_id,
                               std::string* out_error) {
    if (!rpc_client_) {
        if (out_error) *out_error = "ChatSystem not available";
        return false;
    }
    return rpc_client_->RecallMessage(msg_id, user_id, out_error);
}

bool ChatSystem::MarkRead(const std::string& user_id, const std::string& chat_id,
                          int32_t chat_type, const std::string& last_msg_id,
                          std::string* out_error) {
    if (!rpc_client_) {
        if (out_error) *out_error = "ChatSystem not available";
        return false;
    }
    return rpc_client_->MarkRead(user_id, chat_id, chat_type, last_msg_id, out_error);
}

bool ChatSystem::PushToUser(const std::string& user_id, 
                            const std::string& cmd, 
                            const std::string& payload) {
    // 1. 从 SessionStore 获取用户会话信息
    if (!session_store_) {
        return false;
    }
    
    // auto session = session_store_->GetSession(user_id);
    // if (!session) {
    //     return false;  // 用户不在线
    // }
    
    // 2. 调用对应 Gate 的 PushMessage 接口
    // auto gate_client = GetGateClient(session->gate_addr);
    // return gate_client->PushMessage(user_id, cmd, payload);
    
    return false;
}

}  // namespace zone
}  // namespace swift
