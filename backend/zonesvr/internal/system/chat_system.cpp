/**
 * @file chat_system.cpp
 * @brief ChatSystem 实现 - RPC 转发层
 */

#include "chat_system.h"
#include "../rpc/chat_rpc_client.h"
#include "../rpc/gate_rpc_client.h"
#include "../store/session_store.h"

namespace swift {
namespace zone {

ChatSystem::ChatSystem() = default;
ChatSystem::~ChatSystem() = default;

bool ChatSystem::Init() {
    // TODO: 创建 RPC Client，连接 ChatSvr
    // rpc_client_ = std::make_unique<ChatRpcClient>();
    // return rpc_client_->Connect(config.chat_svr_addr);
    return true;
}

void ChatSystem::Shutdown() {
    if (rpc_client_) {
        rpc_client_.reset();
    }
}

// RPC 转发接口的实现示例：
//
// SendMessageResponse ChatSystem::SendMessage(const SendMessageRequest& request) {
//     // 直接转发到 ChatSvr，业务逻辑在 ChatSvr 实现
//     return rpc_client_->SendMessage(request);
// }
//
// CommonResponse ChatSystem::RecallMessage(const RecallMessageRequest& request) {
//     return rpc_client_->RecallMessage(request);
// }

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
