#pragma once

#include <string>
#include <vector>
#include <optional>

namespace swift::chat {

/**
 * 消息存储数据结构
 */
struct MessageData {
    std::string msg_id;
    std::string from_user_id;
    std::string to_id;
    int chat_type = 1;           // 1=私聊, 2=群聊
    std::string content;
    std::string media_url;
    std::string media_type;
    std::vector<std::string> mentions;
    std::string reply_to_msg_id;
    int64_t timestamp = 0;
    int status = 0;              // 0=正常, 1=已撤回
    int64_t recall_at = 0;
};

/**
 * 消息存储接口
 * 
 * RocksDB Key 设计：
 *   msg:{msg_id}                           -> MessageData (JSON)
 *   chat:{chat_id}:{timestamp}:{msg_id}    -> msg_id (时间线索引)
 *   offline:{user_id}:{timestamp}:{msg_id} -> msg_id (离线消息队列)
 */
class MessageStore {
public:
    virtual ~MessageStore() = default;
    
    // 存储消息
    virtual bool Save(const MessageData& msg) = 0;
    
    // 根据 msg_id 查询
    virtual std::optional<MessageData> GetById(const std::string& msg_id) = 0;
    
    // 获取会话历史消息（倒序，支持分页）
    virtual std::vector<MessageData> GetHistory(
        const std::string& chat_id,
        int chat_type,
        const std::string& before_msg_id,
        int limit) = 0;
    
    // 标记消息已撤回
    virtual bool MarkRecalled(const std::string& msg_id, int64_t recall_at) = 0;
    
    // 添加到离线队列
    virtual bool AddToOffline(const std::string& user_id, const std::string& msg_id) = 0;
    
    // 拉取离线消息
    virtual std::vector<MessageData> PullOffline(
        const std::string& user_id,
        const std::string& cursor,
        int limit,
        std::string& next_cursor,
        bool& has_more) = 0;
    
    // 清除离线消息（用户已读后）
    virtual bool ClearOffline(const std::string& user_id, const std::string& until_msg_id) = 0;
};

/**
 * 会话存储
 */
struct ConversationData {
    std::string chat_id;
    int chat_type = 1;
    std::string peer_id;
    std::string last_msg_id;
    int unread_count = 0;
    int64_t updated_at = 0;
    bool is_pinned = false;
    bool is_muted = false;
};

class ConversationStore {
public:
    virtual ~ConversationStore() = default;
    
    virtual bool Upsert(const std::string& user_id, const ConversationData& conv) = 0;
    virtual std::vector<ConversationData> GetList(const std::string& user_id) = 0;
    virtual bool Delete(const std::string& user_id, const std::string& chat_id) = 0;
    virtual bool UpdateUnread(const std::string& user_id, const std::string& chat_id, int delta) = 0;
    virtual bool ClearUnread(const std::string& user_id, const std::string& chat_id) = 0;
};

}  // namespace swift::chat
