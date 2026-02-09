/**
 * @file chat_service.cpp
 * @brief ChatSvr 业务逻辑：发送/撤回/历史/离线/搜索/会话
 */

#include "chat_service.h"
#include "../store/group_store.h"
#include "swift/utils.h"
#include <algorithm>
#include <cstdint>

namespace swift::chat {

ChatServiceCore::ChatServiceCore(std::shared_ptr<MessageStore> msg_store,
                         std::shared_ptr<ConversationStore> conv_store,
                         std::shared_ptr<ConversationRegistry> conv_registry,
                         std::shared_ptr<swift::group_::GroupStore> group_store)
    : msg_store_(std::move(msg_store))
    , conv_store_(std::move(conv_store))
    , conv_registry_(std::move(conv_registry))
    , group_store_(std::move(group_store)) {}

ChatServiceCore::~ChatServiceCore() = default;

std::string ChatServiceCore::GenerateMsgId() {
    return swift::utils::GenerateShortId("m_", 16);
}

std::string ChatServiceCore::ResolveConversationId(const std::string& user_id,
                                                const std::string& chat_id,
                                                ChatType chat_type) {
    if (chat_id.empty()) return "";
    if (chat_type == ChatType::PRIVATE) {
        if (!conv_registry_) return "";
        return conv_registry_->GetOrCreatePrivateConversation(user_id, chat_id);
    }
    return chat_id;  // 群聊 conversation_id = group_id
}

ChatServiceCore::SendResult ChatServiceCore::SendMessage(const std::string& from_user_id,
                                                  const std::string& to_id,
                                                  ChatType chat_type,
                                                  const std::string& content,
                                                  const std::string& media_url,
                                                  const std::string& media_type,
                                                  const std::vector<std::string>& mentions,
                                                  const std::string& reply_to_msg_id) {
    SendResult result{false, "", "", 0, ""};
    if (from_user_id.empty() || to_id.empty()) {
        result.error = "invalid params";
        return result;
    }

    std::string conversation_id;
    if (chat_type == ChatType::PRIVATE) {
        if (!conv_registry_) {
            result.error = "conv_registry not set";
            return result;
        }
        conversation_id = conv_registry_->GetOrCreatePrivateConversation(from_user_id, to_id);
    } else {
        conversation_id = to_id;  // 群聊
        if (group_store_ && !group_store_->IsMember(to_id, from_user_id)) {
            result.error = "not a group member";
            return result;
        }
    }

    int64_t now = swift::utils::GetTimestampMs();
    MessageData msg;
    msg.msg_id = GenerateMsgId();
    msg.from_user_id = from_user_id;
    msg.to_id = to_id;
    msg.conversation_id = conversation_id;
    msg.chat_type = static_cast<int>(chat_type);
    msg.content = content;
    msg.media_url = media_url;
    msg.media_type = media_type.empty() ? "text" : media_type;
    msg.mentions = mentions;
    msg.reply_to_msg_id = reply_to_msg_id;
    msg.timestamp = now;
    msg.status = 0;
    msg.recall_at = 0;

    if (!msg_store_->Save(msg)) {
        result.error = "save failed";
        return result;
    }

    // 更新会话列表：发送方
    ConversationData conv_sender;
    conv_sender.conversation_id = conversation_id;
    conv_sender.chat_type = static_cast<int>(chat_type);
    conv_sender.peer_id = to_id;
    conv_sender.last_msg_id = msg.msg_id;
    conv_sender.updated_at = now;
    conv_store_->Upsert(from_user_id, conv_sender);

    if (chat_type == ChatType::PRIVATE) {
        ConversationData conv_peer;
        conv_peer.conversation_id = conversation_id;
        conv_peer.chat_type = static_cast<int>(chat_type);
        conv_peer.peer_id = from_user_id;
        conv_peer.last_msg_id = msg.msg_id;
        conv_peer.updated_at = now;
        conv_store_->Upsert(to_id, conv_peer);
        conv_store_->UpdateUnread(to_id, conversation_id, 1);
    } else if (group_store_) {
        int total = 0;
        std::vector<swift::group_::GroupMemberData> members =
            group_store_->GetMembers(to_id, 0, 1000, &total);
        for (const auto& m : members) {
            if (m.user_id == from_user_id) continue;
            ConversationData conv_m;
            conv_m.conversation_id = conversation_id;
            conv_m.chat_type = static_cast<int>(chat_type);
            conv_m.peer_id = to_id;
            conv_m.last_msg_id = msg.msg_id;
            conv_m.updated_at = now;
            conv_store_->Upsert(m.user_id, conv_m);
            conv_store_->UpdateUnread(m.user_id, conversation_id, 1);
        }
    }

    // 离线：私聊给 to_id；群聊给除发送者外的所有成员
    if (chat_type == ChatType::PRIVATE) {
        msg_store_->AddToOffline(to_id, msg.msg_id);
    } else if (group_store_) {
        int total = 0;
        std::vector<swift::group_::GroupMemberData> members =
            group_store_->GetMembers(to_id, 0, 1000, &total);
        for (const auto& m : members) {
            if (m.user_id != from_user_id)
                msg_store_->AddToOffline(m.user_id, msg.msg_id);
        }
    }

    result.success = true;
    result.msg_id = msg.msg_id;
    result.conversation_id = conversation_id;
    result.timestamp = now;
    return result;
}

ChatServiceCore::RecallResult ChatServiceCore::RecallMessage(const std::string& msg_id,
                                                      const std::string& user_id) {
    RecallResult result{false, ""};
    if (msg_id.empty() || user_id.empty()) {
        result.error = "invalid params";
        return result;
    }

    auto msg = msg_store_->GetById(msg_id);
    if (!msg) {
        result.error = "message not found";
        return result;
    }
    if (msg->from_user_id != user_id) {
        result.error = "not allowed to recall";
        return result;
    }

    int64_t now = swift::utils::GetTimestampMs();
    if (now - msg->timestamp > static_cast<int64_t>(RECALL_TIMEOUT_SECONDS) * 1000) {
        result.error = "recall timeout";
        return result;
    }

    if (!msg_store_->MarkRecalled(msg_id, now)) {
        result.error = "mark recalled failed";
        return result;
    }
    result.success = true;
    return result;
}

ChatServiceCore::OfflineResult ChatServiceCore::PullOffline(const std::string& user_id,
                                                    const std::string& cursor,
                                                    int limit) {
    OfflineResult result;
    result.messages = msg_store_->PullOffline(user_id, cursor,
                                              limit <= 0 ? 100 : limit,
                                              result.next_cursor, result.has_more);
    return result;
}

std::vector<MessageData> ChatServiceCore::GetHistory(const std::string& user_id,
                                                  const std::string& chat_id,
                                                  ChatType chat_type,
                                                  const std::string& before_msg_id,
                                                  int limit) {
    if (chat_id.empty() || limit <= 0) return {};
    std::string conversation_id = ResolveConversationId(user_id, chat_id, chat_type);
    if (conversation_id.empty()) return {};
    if (chat_type == ChatType::GROUP && group_store_) {
        auto g = group_store_->GetGroup(conversation_id);
        if (g && g->status == 1)
            return {};  // 已解散群聊，用户侧视为无历史
    }
    return msg_store_->GetHistory(conversation_id, static_cast<int>(chat_type), before_msg_id, limit);
}

bool ChatServiceCore::MarkRead(const std::string& user_id, const std::string& chat_id,
                           ChatType chat_type, const std::string& last_msg_id) {
    if (user_id.empty() || chat_id.empty()) return false;
    std::string conversation_id = ResolveConversationId(user_id, chat_id, chat_type);
    if (conversation_id.empty()) return false;

    bool ok = conv_store_->ClearUnread(user_id, conversation_id);
    if (!last_msg_id.empty())
        msg_store_->ClearOffline(user_id, last_msg_id);
    return ok;
}

std::vector<ConversationData> ChatServiceCore::SyncConversations(const std::string& user_id) {
    if (user_id.empty()) return {};
    std::vector<ConversationData> list = conv_store_->GetList(user_id);
    if (!group_store_) return list;
    std::vector<ConversationData> out;
    for (auto& c : list) {
        if (c.chat_type == static_cast<int>(ChatType::GROUP)) {
            auto g = group_store_->GetGroup(c.conversation_id);
            if (g && g->status == 1) continue;  // 已解散群不展示
        }
        out.push_back(std::move(c));
    }
    return out;
}

std::optional<MessageData> ChatServiceCore::GetMessageById(const std::string& msg_id) {
    if (msg_id.empty()) return std::nullopt;
    return msg_store_->GetById(msg_id);
}

ChatServiceCore::DeleteConversationResult ChatServiceCore::DeleteConversation(
    const std::string& user_id, const std::string& chat_id, ChatType chat_type) {
    DeleteConversationResult result{false, swift::ErrorCode::OK};
    if (user_id.empty() || chat_id.empty()) {
        result.error = swift::ErrorCode::INVALID_PARAM;
        return result;
    }
    if (chat_type == ChatType::PRIVATE) {
        result.error = swift::ErrorCode::CONVERSATION_PRIVATE_CANNOT_DELETE;
        return result;
    }
    if (chat_type != ChatType::GROUP || !group_store_) {
        result.error = swift::ErrorCode::INVALID_PARAM;
        return result;
    }
    auto g = group_store_->GetGroup(chat_id);
    if (!g) {
        result.error = swift::ErrorCode::GROUP_NOT_FOUND;
        return result;
    }
    if (g->owner_id != user_id) {
        result.error = swift::ErrorCode::NOT_GROUP_OWNER;
        return result;
    }
    if (g->status == 1) {
        result.success = true;
        return result;
    }
    int total = 0;
    std::vector<swift::group_::GroupMemberData> members =
        group_store_->GetMembers(chat_id, 0, 10000, &total);
    if (!group_store_->DissolveGroup(chat_id)) {
        result.error = swift::ErrorCode::INTERNAL_ERROR;
        return result;
    }
    for (const auto& m : members)
        conv_store_->Delete(m.user_id, chat_id);
    result.success = true;
    return result;
}

}  // namespace swift::chat
