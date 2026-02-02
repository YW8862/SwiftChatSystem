#pragma once

#include <string>
#include <vector>
#include <memory>
#include "../store/message_store.h"

namespace swift::chat {

/**
 * 消息服务业务逻辑
 */
class ChatService {
public:
    ChatService(std::shared_ptr<MessageStore> msg_store,
                std::shared_ptr<ConversationStore> conv_store);
    ~ChatService();
    
    // 发送消息
    struct SendResult {
        bool success;
        std::string msg_id;
        int64_t timestamp;
        std::string error;
    };
    SendResult SendMessage(const std::string& from_user_id, const std::string& to_id,
                           int chat_type, const std::string& content,
                           const std::string& media_url, const std::string& media_type,
                           const std::vector<std::string>& mentions);
    
    // 撤回消息（2分钟内）
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
    
    // 搜索消息
    std::vector<MessageData> SearchMessages(const std::string& user_id,
                                            const std::string& keyword,
                                            const std::string& chat_id, int limit);
    
    // 获取历史消息
    std::vector<MessageData> GetHistory(const std::string& chat_id, int chat_type,
                                        const std::string& before_msg_id, int limit);
    
    // 标记已读
    bool MarkRead(const std::string& user_id, const std::string& chat_id,
                  int chat_type, const std::string& last_msg_id);
    
private:
    std::string GenerateMsgId();
    std::string BuildChatId(const std::string& user1, const std::string& user2);
    
    std::shared_ptr<MessageStore> msg_store_;
    std::shared_ptr<ConversationStore> conv_store_;
    
    static constexpr int RECALL_TIMEOUT_SECONDS = 120;  // 2分钟
};

}  // namespace swift::chat
