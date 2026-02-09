/**
 * @file chat_rpc_client.cpp
 * @brief ChatRpcClient 实现
 */

#include "chat_rpc_client.h"

namespace swift {
namespace zone {

void ChatRpcClient::InitStub() {
    stub_ = swift::chat::ChatService::NewStub(GetChannel());
}

static ChatMessageResult FromProto(const swift::chat::ChatMessage& m) {
    ChatMessageResult r;
    r.msg_id = m.msg_id();
    r.from_user_id = m.from_user_id();
    r.to_id = m.to_id();
    r.chat_type = m.chat_type();
    r.content = m.content();
    r.media_url = m.media_url();
    r.media_type = m.media_type();
    r.timestamp = m.timestamp();
    r.status = m.status();
    return r;
}

SendMessageResult ChatRpcClient::SendMessage(const std::string& from_user_id, const std::string& to_id,
                                             int32_t chat_type, const std::string& content,
                                             const std::string& media_url, const std::string& media_type,
                                             const std::vector<std::string>& mentions,
                                             const std::string& reply_to_msg_id,
                                             const std::string& client_msg_id, int64_t file_size) {
    SendMessageResult result;
    if (!stub_) return result;
    swift::chat::SendMessageRequest req;
    req.set_from_user_id(from_user_id);
    req.set_to_id(to_id);
    req.set_chat_type(chat_type);
    req.set_content(content);
    if (!media_url.empty()) req.set_media_url(media_url);
    if (!media_type.empty()) req.set_media_type(media_type);
    for (const auto& u : mentions) req.add_mentions(u);
    if (!reply_to_msg_id.empty()) req.set_reply_to_msg_id(reply_to_msg_id);
    if (!client_msg_id.empty()) req.set_client_msg_id(client_msg_id);
    if (file_size > 0) req.set_file_size(file_size);
    swift::chat::SendMessageResponse resp;
    auto ctx = CreateContext(10000);
    grpc::Status status = stub_->SendMessage(ctx.get(), req, &resp);
    if (!status.ok()) {
        result.error = status.error_message();
        return result;
    }
    if (resp.code() != 0) {
        result.error = resp.message().empty() ? "send failed" : resp.message();
        return result;
    }
    result.success = true;
    result.msg_id = resp.msg_id();
    result.timestamp = resp.timestamp();
    return result;
}

bool ChatRpcClient::RecallMessage(const std::string& msg_id, const std::string& user_id,
                                  std::string* out_error) {
    if (!stub_) return false;
    swift::chat::RecallMessageRequest req;
    req.set_msg_id(msg_id);
    req.set_user_id(user_id);
    swift::chat::RecallMessageResponse resp;
    auto ctx = CreateContext(5000);
    grpc::Status status = stub_->RecallMessage(ctx.get(), req, &resp);
    if (!status.ok()) {
        if (out_error) *out_error = status.error_message();
        return false;
    }
    if (resp.code() != 0 && out_error)
        *out_error = resp.message().empty() ? "recall failed" : resp.message();
    return resp.code() == 0;
}

bool ChatRpcClient::PullOffline(const std::string& user_id, int32_t limit, const std::string& cursor,
                                std::vector<ChatMessageResult>* out_messages, std::string* next_cursor,
                                bool* has_more, std::string* out_error) {
    if (!stub_) return false;
    swift::chat::PullOfflineRequest req;
    req.set_user_id(user_id);
    if (limit > 0) req.set_limit(limit);
    if (!cursor.empty()) req.set_cursor(cursor);
    swift::chat::PullOfflineResponse resp;
    auto ctx = CreateContext(10000);
    grpc::Status status = stub_->PullOffline(ctx.get(), req, &resp);
    if (!status.ok()) {
        if (out_error) *out_error = status.error_message();
        return false;
    }
    if (resp.code() != 0 && out_error)
        *out_error = resp.message().empty() ? "pull failed" : resp.message();
    if (resp.code() != 0) return false;
    if (out_messages) {
        out_messages->clear();
        for (const auto& m : resp.messages()) out_messages->push_back(FromProto(m));
    }
    if (next_cursor) *next_cursor = resp.next_cursor();
    if (has_more) *has_more = resp.has_more();
    return true;
}

bool ChatRpcClient::GetHistory(const std::string& user_id, const std::string& chat_id, int32_t chat_type,
                               const std::string& before_msg_id, int32_t limit,
                               std::vector<ChatMessageResult>* out_messages, bool* has_more,
                               std::string* out_error) {
    if (!stub_) return false;
    swift::chat::GetHistoryRequest req;
    req.set_user_id(user_id);
    req.set_chat_id(chat_id);
    req.set_chat_type(chat_type);
    if (!before_msg_id.empty()) req.set_before_msg_id(before_msg_id);
    if (limit > 0) req.set_limit(limit);
    swift::chat::GetHistoryResponse resp;
    auto ctx = CreateContext(10000);
    grpc::Status status = stub_->GetHistory(ctx.get(), req, &resp);
    if (!status.ok()) {
        if (out_error) *out_error = status.error_message();
        return false;
    }
    if (resp.code() != 0 && out_error)
        *out_error = resp.message().empty() ? "get history failed" : resp.message();
    if (resp.code() != 0) return false;
    if (out_messages) {
        out_messages->clear();
        for (const auto& m : resp.messages()) out_messages->push_back(FromProto(m));
    }
    if (has_more) *has_more = resp.has_more();
    return true;
}

bool ChatRpcClient::SyncConversations(const std::string& user_id, int64_t last_sync_time,
                                      std::vector<ConversationResult>* out_conversations,
                                      std::string* out_error) {
    if (!stub_) return false;
    swift::chat::SyncConversationsRequest req;
    req.set_user_id(user_id);
    if (last_sync_time > 0) req.set_last_sync_time(last_sync_time);
    swift::chat::SyncConversationsResponse resp;
    auto ctx = CreateContext(10000);
    grpc::Status status = stub_->SyncConversations(ctx.get(), req, &resp);
    if (!status.ok()) {
        if (out_error) *out_error = status.error_message();
        return false;
    }
    if (resp.code() != 0 && out_error)
        *out_error = resp.message().empty() ? "sync conversations failed" : resp.message();
    if (resp.code() != 0) return false;
    if (out_conversations) {
        out_conversations->clear();
        for (const auto& c : resp.conversations()) {
            ConversationResult r;
            r.chat_id = c.chat_id();
            r.chat_type = c.chat_type();
            r.peer_id = c.peer_id();
            r.peer_name = c.peer_name();
            r.peer_avatar = c.peer_avatar();
            r.unread_count = c.unread_count();
            r.updated_at = c.updated_at();
            if (c.has_last_message()) {
                r.last_msg_id = c.last_message().msg_id();
                r.last_content = c.last_message().content();
                r.last_timestamp = c.last_message().timestamp();
            }
            out_conversations->push_back(r);
        }
    }
    return true;
}

bool ChatRpcClient::DeleteConversation(const std::string& user_id, const std::string& chat_id,
                                       int32_t chat_type, std::string* out_error) {
    if (!stub_) return false;
    swift::chat::DeleteConversationRequest req;
    req.set_user_id(user_id);
    req.set_chat_id(chat_id);
    req.set_chat_type(chat_type);
    swift::chat::DeleteConversationResponse resp;
    auto ctx = CreateContext(5000);
    grpc::Status status = stub_->DeleteConversation(ctx.get(), req, &resp);
    if (!status.ok()) {
        if (out_error) *out_error = status.error_message();
        return false;
    }
    if (resp.code() != 0 && out_error)
        *out_error = resp.message().empty() ? "delete conversation failed" : resp.message();
    return resp.code() == 0;
}

bool ChatRpcClient::MarkRead(const std::string& user_id, const std::string& chat_id, int32_t chat_type,
                             const std::string& last_msg_id, std::string* out_error) {
    if (!stub_) return false;
    swift::chat::MarkReadRequest req;
    req.set_user_id(user_id);
    req.set_chat_id(chat_id);
    req.set_chat_type(chat_type);
    if (!last_msg_id.empty()) req.set_last_msg_id(last_msg_id);
    swift::chat::MarkReadResponse resp;
    auto ctx = CreateContext(5000);
    grpc::Status status = stub_->MarkRead(ctx.get(), req, &resp);
    if (!status.ok()) {
        if (out_error) *out_error = status.error_message();
        return false;
    }
    if (resp.code() != 0 && out_error)
        *out_error = resp.message().empty() ? "mark read failed" : resp.message();
    return resp.code() == 0;
}

}  // namespace zone
}  // namespace swift
