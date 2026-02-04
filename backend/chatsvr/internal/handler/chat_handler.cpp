/**
 * @file chat_handler.cpp
 * @brief 消息 gRPC API：将请求转调 ChatServiceCore，结果写入 proto 响应。
 * - 请求校验：必填参数为空时返回 INVALID_PARAM
 * - limit 上限：PullOffline 200、SearchMessages 100、GetHistory 100
 * - SyncConversations 填充 last_message（通过 GetMessageById）
 */

#include "chat_handler.h"
#include "../service/chat_service.h"
#include "swift/error_code.h"
#include "swift/grpc_auth.h"
#include <grpcpp/server_context.h>
#include <algorithm>
#include <optional>

namespace swift::chat {

namespace {
constexpr int kMaxPullOfflineLimit = 200;
constexpr int kMaxSearchLimit = 100;
constexpr int kMaxHistoryLimit = 100;
}  // namespace

ChatHandler::ChatHandler(std::shared_ptr<ChatServiceCore> service,
                         const std::string& jwt_secret)
    : service_(std::move(service)), jwt_secret_(jwt_secret) {}

ChatHandler::~ChatHandler() = default;

namespace {

void SetCommonOk(::swift::common::CommonResponse* response) {
    response->set_code(static_cast<int>(swift::ErrorCode::OK));
    response->set_message(swift::ErrorCodeToString(swift::ErrorCode::OK));
}

void SetCommonFail(::swift::common::CommonResponse* response,
                   swift::ErrorCode code,
                   const std::string& message = "") {
    response->set_code(swift::ErrorCodeToInt(code));
    response->set_message(message.empty() ? swift::ErrorCodeToString(code) : message);
}

swift::ErrorCode MapSendErrorToCode(const std::string& error) {
    if (error.empty()) return swift::ErrorCode::OK;
    if (error == "invalid params") return swift::ErrorCode::INVALID_PARAM;
    if (error == "not a group member") return swift::ErrorCode::NOT_GROUP_MEMBER;
    if (error == "save failed" || error == "conv_registry not set") return swift::ErrorCode::MSG_SEND_FAILED;
    return swift::ErrorCode::INTERNAL_ERROR;
}

swift::ErrorCode MapRecallErrorToCode(const std::string& error) {
    if (error.empty()) return swift::ErrorCode::OK;
    if (error == "invalid params") return swift::ErrorCode::INVALID_PARAM;
    if (error == "message not found") return swift::ErrorCode::MSG_NOT_FOUND;
    if (error == "not allowed to recall") return swift::ErrorCode::RECALL_NOT_ALLOWED;
    if (error == "recall timeout") return swift::ErrorCode::RECALL_TIMEOUT;
    if (error == "mark recalled failed") return swift::ErrorCode::INTERNAL_ERROR;
    return swift::ErrorCode::INTERNAL_ERROR;
}

void FillChatMessage(::swift::chat::ChatMessage* out, const MessageData& m) {
    out->set_msg_id(m.msg_id);
    out->set_from_user_id(m.from_user_id);
    out->set_to_id(m.to_id);
    out->set_chat_type(m.chat_type);
    out->set_content(m.content);
    out->set_media_url(m.media_url);
    out->set_media_type(m.media_type);
    for (const auto& mention : m.mentions) out->add_mentions(mention);
    out->set_reply_to_msg_id(m.reply_to_msg_id);
    out->set_timestamp(m.timestamp);
    out->set_status(m.status);
    out->set_recall_at(m.recall_at);
}

void FillConversation(::swift::chat::Conversation* out, const ConversationData& c,
                      const MessageData* last_msg) {
    out->set_chat_id(c.conversation_id);
    out->set_chat_type(c.chat_type);
    out->set_peer_id(c.peer_id);
    out->set_peer_name("");   // 由客户端或用户服务补充
    out->set_peer_avatar(""); // 由客户端或用户服务补充
    if (last_msg) {
        FillChatMessage(out->mutable_last_message(), *last_msg);
    }
    out->set_unread_count(c.unread_count);
    out->set_updated_at(c.updated_at);
    out->set_is_pinned(c.is_pinned);
    out->set_is_muted(c.is_muted);
}

}  // namespace

::grpc::Status ChatHandler::SendMessage(::grpc::ServerContext* context,
                                         const ::swift::chat::SendMessageRequest* request,
                                         ::swift::chat::SendMessageResponse* response) {
    std::string uid = swift::GetAuthenticatedUserId(context, jwt_secret_);
    if (uid.empty()) {
        response->set_code(swift::ErrorCodeToInt(swift::ErrorCode::TOKEN_INVALID));
        response->set_message("token invalid or missing");
        return ::grpc::Status::OK;
    }
    if (request->to_id().empty()) {
        response->set_code(swift::ErrorCodeToInt(swift::ErrorCode::INVALID_PARAM));
        response->set_message(swift::ErrorCodeToString(swift::ErrorCode::INVALID_PARAM));
        return ::grpc::Status::OK;
    }
    std::vector<std::string> mentions(request->mentions().begin(), request->mentions().end());
    ChatType ctype = request->chat_type() == 2 ? ChatType::GROUP : ChatType::PRIVATE;
    auto result = service_->SendMessage(
        uid, request->to_id(), ctype,
        request->content(), request->media_url(), request->media_type(),
        mentions, request->reply_to_msg_id());
    if (result.success) {
        response->set_code(static_cast<int>(swift::ErrorCode::OK));
        response->set_message(swift::ErrorCodeToString(swift::ErrorCode::OK));
        response->set_msg_id(result.msg_id);
        response->set_timestamp(result.timestamp);
    } else {
        swift::ErrorCode code = MapSendErrorToCode(result.error);
        response->set_code(swift::ErrorCodeToInt(code));
        response->set_message(result.error.empty() ? swift::ErrorCodeToString(code) : result.error);
    }
    return ::grpc::Status::OK;
}

::grpc::Status ChatHandler::RecallMessage(::grpc::ServerContext* context,
                                           const ::swift::chat::RecallMessageRequest* request,
                                           ::swift::common::CommonResponse* response) {
    std::string uid = swift::GetAuthenticatedUserId(context, jwt_secret_);
    if (uid.empty()) {
        SetCommonFail(response, swift::ErrorCode::TOKEN_INVALID);
        return ::grpc::Status::OK;
    }
    if (request->msg_id().empty()) {
        SetCommonFail(response, swift::ErrorCode::INVALID_PARAM);
        return ::grpc::Status::OK;
    }
    auto result = service_->RecallMessage(request->msg_id(), uid);
    if (result.success)
        SetCommonOk(response);
    else
        SetCommonFail(response, MapRecallErrorToCode(result.error), result.error);
    return ::grpc::Status::OK;
}

::grpc::Status ChatHandler::PullOffline(::grpc::ServerContext* context,
                                         const ::swift::chat::PullOfflineRequest* request,
                                         ::swift::chat::PullOfflineResponse* response) {
    std::string uid = swift::GetAuthenticatedUserId(context, jwt_secret_);
    if (uid.empty()) {
        response->set_code(swift::ErrorCodeToInt(swift::ErrorCode::TOKEN_INVALID));
        response->set_message("token invalid or missing");
        return ::grpc::Status::OK;
    }
    int limit = request->limit() > 0 ? std::min(request->limit(), kMaxPullOfflineLimit) : 100;
    auto result = service_->PullOffline(uid, request->cursor(), limit);
    response->set_code(static_cast<int>(swift::ErrorCode::OK));
    response->set_message(swift::ErrorCodeToString(swift::ErrorCode::OK));
    response->set_next_cursor(result.next_cursor);
    response->set_has_more(result.has_more);
    for (const auto& m : result.messages) {
        FillChatMessage(response->add_messages(), m);
    }
    return ::grpc::Status::OK;
}

::grpc::Status ChatHandler::SearchMessages(::grpc::ServerContext* context,
                                             const ::swift::chat::SearchMessagesRequest* request,
                                             ::swift::chat::SearchMessagesResponse* response) {
    std::string uid = swift::GetAuthenticatedUserId(context, jwt_secret_);
    if (uid.empty()) {
        response->set_code(swift::ErrorCodeToInt(swift::ErrorCode::TOKEN_INVALID));
        response->set_message("token invalid or missing");
        return ::grpc::Status::OK;
    }
    int limit = request->limit() > 0 ? std::min(request->limit(), kMaxSearchLimit) : 20;
    ChatType ctype = request->chat_type() == 2 ? ChatType::GROUP : ChatType::PRIVATE;
    auto messages = service_->SearchMessages(
        uid, request->keyword(), request->chat_id(), ctype, limit);
    response->set_code(static_cast<int>(swift::ErrorCode::OK));
    response->set_message(swift::ErrorCodeToString(swift::ErrorCode::OK));
    response->set_total(static_cast<int32_t>(messages.size()));
    for (const auto& m : messages) {
        FillChatMessage(response->add_messages(), m);
    }
    return ::grpc::Status::OK;
}

::grpc::Status ChatHandler::MarkRead(::grpc::ServerContext* context,
                                      const ::swift::chat::MarkReadRequest* request,
                                      ::swift::common::CommonResponse* response) {
    std::string uid = swift::GetAuthenticatedUserId(context, jwt_secret_);
    if (uid.empty()) {
        SetCommonFail(response, swift::ErrorCode::TOKEN_INVALID);
        return ::grpc::Status::OK;
    }
    if (request->chat_id().empty()) {
        SetCommonFail(response, swift::ErrorCode::INVALID_PARAM);
        return ::grpc::Status::OK;
    }
    ChatType ctype = request->chat_type() == 2 ? ChatType::GROUP : ChatType::PRIVATE;
    bool ok = service_->MarkRead(uid, request->chat_id(), ctype, request->last_msg_id());
    if (ok)
        SetCommonOk(response);
    else
        SetCommonFail(response, swift::ErrorCode::CONVERSATION_NOT_FOUND);
    return ::grpc::Status::OK;
}

::grpc::Status ChatHandler::GetHistory(::grpc::ServerContext* context,
                                        const ::swift::chat::GetHistoryRequest* request,
                                        ::swift::chat::GetHistoryResponse* response) {
    std::string uid = swift::GetAuthenticatedUserId(context, jwt_secret_);
    if (uid.empty()) {
        response->set_code(swift::ErrorCodeToInt(swift::ErrorCode::TOKEN_INVALID));
        response->set_message("token invalid or missing");
        return ::grpc::Status::OK;
    }
    if (request->chat_id().empty()) {
        response->set_code(swift::ErrorCodeToInt(swift::ErrorCode::INVALID_PARAM));
        response->set_message(swift::ErrorCodeToString(swift::ErrorCode::INVALID_PARAM));
        return ::grpc::Status::OK;
    }
    int limit = request->limit() > 0 ? std::min(request->limit(), kMaxHistoryLimit) : 50;
    ChatType ctype = request->chat_type() == 2 ? ChatType::GROUP : ChatType::PRIVATE;
    auto messages = service_->GetHistory(uid, request->chat_id(), ctype,
                                          request->before_msg_id(), limit);
    response->set_code(static_cast<int>(swift::ErrorCode::OK));
    response->set_message(swift::ErrorCodeToString(swift::ErrorCode::OK));
    response->set_has_more(static_cast<int>(messages.size()) == limit);
    for (const auto& m : messages) {
        FillChatMessage(response->add_messages(), m);
    }
    return ::grpc::Status::OK;
}

::grpc::Status ChatHandler::SyncConversations(::grpc::ServerContext* context,
                                               const ::swift::chat::SyncConversationsRequest* request,
                                               ::swift::chat::SyncConversationsResponse* response) {
    (void)request->last_sync_time();  // 可选：后续可做增量同步
    std::string uid = swift::GetAuthenticatedUserId(context, jwt_secret_);
    if (uid.empty()) {
        response->set_code(swift::ErrorCodeToInt(swift::ErrorCode::TOKEN_INVALID));
        response->set_message("token invalid or missing");
        return ::grpc::Status::OK;
    }
    auto convs = service_->SyncConversations(uid);
    response->set_code(static_cast<int>(swift::ErrorCode::OK));
    response->set_message(swift::ErrorCodeToString(swift::ErrorCode::OK));
    for (const auto& c : convs) {
        const MessageData* last_msg_ptr = nullptr;
        std::optional<MessageData> last_msg;
        if (!c.last_msg_id.empty()) {
            last_msg = service_->GetMessageById(c.last_msg_id);
            if (last_msg) last_msg_ptr = &*last_msg;
        }
        FillConversation(response->add_conversations(), c, last_msg_ptr);
    }
    return ::grpc::Status::OK;
}

::grpc::Status ChatHandler::DeleteConversation(::grpc::ServerContext* context,
                                                const ::swift::chat::DeleteConversationRequest* request,
                                                ::swift::common::CommonResponse* response) {
    std::string uid = swift::GetAuthenticatedUserId(context, jwt_secret_);
    if (uid.empty()) {
        SetCommonFail(response, swift::ErrorCode::TOKEN_INVALID);
        return ::grpc::Status::OK;
    }
    if (request->chat_id().empty()) {
        SetCommonFail(response, swift::ErrorCode::INVALID_PARAM);
        return ::grpc::Status::OK;
    }
    ChatType ctype = request->chat_type() == 2 ? ChatType::GROUP : ChatType::PRIVATE;
    auto result = service_->DeleteConversation(uid, request->chat_id(), ctype);
    if (result.success)
        SetCommonOk(response);
    else
        SetCommonFail(response, result.error);
    return ::grpc::Status::OK;
}

}  // namespace swift::chat
