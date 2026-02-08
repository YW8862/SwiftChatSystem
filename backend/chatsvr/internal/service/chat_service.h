#pragma once

#include <string>
#include <vector>
#include <memory>
#include "../store/message_store.h"
#include "swift/chat_type.h"
#include "swift/error_code.h"

namespace swift::group_ {
class GroupStore;
}

namespace swift::chat {

using ChatType = swift::ChatType;

/**
 * 消息服务业务逻辑（类名带 Core 以区分 proto 生成的 ChatService）。
 * 依赖：MessageStore、ConversationStore、ConversationRegistry；群聊离线需 GroupStore。
 */
class ChatServiceCore {
public:
    ChatServiceCore(std::shared_ptr<MessageStore> msg_store,
                std::shared_ptr<ConversationStore> conv_store,
                std::shared_ptr<ConversationRegistry> conv_registry,
                std::shared_ptr<swift::group_::GroupStore> group_store = nullptr);
    ~ChatServiceCore();

    // 发送消息（私聊用 GetOrCreatePrivateConversation 得到 conversation_id，群聊用 to_id）
    struct SendResult {
        bool success;
        std::string msg_id;
        std::string conversation_id;
        int64_t timestamp;
        std::string error;
    };
    SendResult SendMessage(const std::string& from_user_id, const std::string& to_id,
                           ChatType chat_type, const std::string& content,
                           const std::string& media_url, const std::string& media_type,
                           const std::vector<std::string>& mentions,
                           const std::string& reply_to_msg_id = "");

    // 撤回消息（2 分钟内，仅发送者）
    struct RecallResult {
        bool success;
        std::string error;
    };
    RecallResult RecallMessage(const std::string& msg_id, const std::string& user_id);

    // 拉取离线消息
    struct OfflineResult {
        std::vector<MessageData> messages;
        std::string next_cursor;
        bool has_more;
    };
    OfflineResult PullOffline(const std::string& user_id, const std::string& cursor, int limit);

    // 搜索消息（keyword 匹配 content；chat_id 为空则搜当前用户各会话，有值则限定该会话）
    std::vector<MessageData> SearchMessages(const std::string& user_id,
                                            const std::string& keyword,
                                            const std::string& chat_id, ChatType chat_type,
                                            int limit = 20);

    // 获取历史消息（chat_id 对私聊为对方 user_id，对群聊为 group_id；内部解析为 conversation_id）
    std::vector<MessageData> GetHistory(const std::string& user_id,
                                        const std::string& chat_id, ChatType chat_type,
                                        const std::string& before_msg_id, int limit);

    // 标记已读：清除该会话未读数，并清除离线队列到 last_msg_id
    bool MarkRead(const std::string& user_id, const std::string& chat_id,
                  ChatType chat_type, const std::string& last_msg_id);

    // 同步会话列表
    std::vector<ConversationData> SyncConversations(const std::string& user_id);

    // 根据 msg_id 查询单条消息（供 Handler 填充会话 last_message 等）
    std::optional<MessageData> GetMessageById(const std::string& msg_id);

    // 删除会话：私聊不允许（返回 CONVERSATION_PRIVATE_CANNOT_DELETE）；群聊仅群主可操作，解散群并移除所有人会话入口，消息仍存服务器，群 ID 不可复用
    struct DeleteConversationResult {
        bool success;
        swift::ErrorCode error;
    };
    DeleteConversationResult DeleteConversation(const std::string& user_id, const std::string& chat_id, ChatType chat_type);

private:
    std::string GenerateMsgId();
    // 解析为 store 使用的 conversation_id：私聊=GetOrCreatePrivateConversation，群聊=chat_id
    std::string ResolveConversationId(const std::string& user_id, const std::string& chat_id, ChatType chat_type);

    std::shared_ptr<MessageStore> msg_store_;
    std::shared_ptr<ConversationStore> conv_store_;
    std::shared_ptr<ConversationRegistry> conv_registry_;
    std::shared_ptr<swift::group_::GroupStore> group_store_;

    static constexpr int RECALL_TIMEOUT_SECONDS = 120;  // 2 分钟
    static constexpr int SEARCH_HISTORY_LIMIT = 500;
};

}  // namespace swift::chat
