/**
 * @file chat_rpc_client.h
 * @brief ChatSvr gRPC 客户端
 */

#pragma once

#include "rpc_client_base.h"
#include "chat.grpc.pb.h"
#include <memory>
#include <string>
#include <vector>
#include <cstdint>

namespace swift {
namespace zone {

struct ChatMessageResult {
    std::string msg_id;
    std::string from_user_id;
    std::string to_id;
    int32_t chat_type = 0;
    std::string content;
    std::string media_url;
    std::string media_type;
    int64_t timestamp = 0;
    int32_t status = 0;
};

struct SendMessageResult {
    bool success = false;
    std::string msg_id;
    int64_t timestamp = 0;
    std::string error;
};

struct ConversationResult {
    std::string chat_id;
    int32_t chat_type = 0;
    std::string peer_id;
    std::string peer_name;
    std::string peer_avatar;
    int32_t unread_count = 0;
    int64_t updated_at = 0;
    std::string last_msg_id;
    std::string last_content;
    int64_t last_timestamp = 0;
};

/**
 * @class ChatRpcClient
 * @brief ChatSvr 的 gRPC 客户端封装
 */
class ChatRpcClient : public RpcClientBase {
public:
    ChatRpcClient() = default;
    ~ChatRpcClient() override = default;

    void InitStub();

    SendMessageResult SendMessage(const std::string& from_user_id, const std::string& to_id,
                                  int32_t chat_type, const std::string& content,
                                  const std::string& media_url, const std::string& media_type,
                                  const std::vector<std::string>& mentions,
                                  const std::string& reply_to_msg_id,
                                  const std::string& client_msg_id, int64_t file_size);
    bool RecallMessage(const std::string& msg_id, const std::string& user_id, std::string* out_error);
    bool PullOffline(const std::string& user_id, int32_t limit, const std::string& cursor,
                     std::vector<ChatMessageResult>* out_messages, std::string* next_cursor,
                     bool* has_more, std::string* out_error);
    bool GetHistory(const std::string& user_id, const std::string& chat_id, int32_t chat_type,
                    const std::string& before_msg_id, int32_t limit,
                    std::vector<ChatMessageResult>* out_messages, bool* has_more, std::string* out_error);
    bool SyncConversations(const std::string& user_id, int64_t last_sync_time,
                           std::vector<ConversationResult>* out_conversations, std::string* out_error);
    bool DeleteConversation(const std::string& user_id, const std::string& chat_id, int32_t chat_type,
                            std::string* out_error);
    bool MarkRead(const std::string& user_id, const std::string& chat_id, int32_t chat_type,
                  const std::string& last_msg_id, std::string* out_error);

private:
    std::unique_ptr<swift::chat::ChatService::Stub> stub_;
};

}  // namespace zone
}  // namespace swift
