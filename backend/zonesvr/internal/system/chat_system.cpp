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
    if (!rpc_client_->Connect(config_->chat_svr_addr)) return false;
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
                                                      const std::string& client_msg_id,
                                                      int64_t file_size) {
    SendMessageResult result;
    if (!rpc_client_) return result;
    auto r = rpc_client_->SendMessage(from_user_id, to_id, chat_type, content,
                                      media_url, media_type, client_msg_id, file_size);
    result.success = r.success;
    result.msg_id = r.msg_id;
    result.timestamp = r.timestamp;
    result.error = r.error;
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
