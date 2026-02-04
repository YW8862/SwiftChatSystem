#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace swift::chat {

/**
 * 统一会话模型：私聊 = 两人会话（conversation_id 由 GetOrCreatePrivateConversation 得到），
 * 群聊 = 多人会话（conversation_id = group_id）。消息统一按 conversation_id 存储。
 */
struct MessageData {
    std::string msg_id;
    std::string from_user_id;
    std::string to_id;
    std::string conversation_id; // 会话 ID（私聊=p_u1_u2，群聊=group_id）
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
 *   msg:{msg_id}                                    -> MessageData (JSON)
 *   chat:{conversation_id}:{rev_ts}:{msg_id}         -> "" (时间线索引)
 *   conv_meta:{conversation_id}                      -> 私聊会话元信息（type=private）
 *   offline:{user_id}:{rev_ts}:{msg_id}             -> "" (离线队列)
 */
class MessageStore {
public:
    virtual ~MessageStore() = default;
    
    // 存储消息
    virtual bool Save(const MessageData& msg) = 0;
    
    // 根据 msg_id 查询
    virtual std::optional<MessageData> GetById(const std::string& msg_id) = 0;
    
    // 获取会话历史消息（倒序，支持分页）；conversation_id = 私聊 id 或 group_id
    virtual std::vector<MessageData> GetHistory(
        const std::string& conversation_id,
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

/** RocksDB 消息存储实现 */
class RocksDBMessageStore : public MessageStore {
public:
    explicit RocksDBMessageStore(const std::string& db_path);
    ~RocksDBMessageStore() override;

    bool Save(const MessageData& msg) override;
    std::optional<MessageData> GetById(const std::string& msg_id) override;
    std::vector<MessageData> GetHistory(const std::string& conversation_id, int chat_type,
                                         const std::string& before_msg_id, int limit) override;
    bool MarkRecalled(const std::string& msg_id, int64_t recall_at) override;
    bool AddToOffline(const std::string& user_id, const std::string& msg_id) override;
    std::vector<MessageData> PullOffline(const std::string& user_id, const std::string& cursor,
                                          int limit, std::string& next_cursor, bool& has_more) override;
    bool ClearOffline(const std::string& user_id, const std::string& until_msg_id) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * 会话注册：私聊 get-or-create，保证两好友间唯一会话 id
 */
class ConversationRegistry {
public:
    virtual ~ConversationRegistry() = default;
    virtual std::string GetOrCreatePrivateConversation(const std::string& user_id_1,
                                                        const std::string& user_id_2) = 0;
};

/** RocksDB 实现：conv_meta:p_u1_u2 -> {"type":"private"} */
class RocksDBConversationRegistry : public ConversationRegistry {
public:
    explicit RocksDBConversationRegistry(const std::string& db_path);
    ~RocksDBConversationRegistry() override;
    std::string GetOrCreatePrivateConversation(const std::string& user_id_1,
                                                const std::string& user_id_2) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * 用户侧会话列表（每个用户看到的会话摘要）
 */
struct ConversationData {
    std::string conversation_id;
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
    virtual bool Delete(const std::string& user_id, const std::string& conversation_id) = 0;
    virtual bool UpdateUnread(const std::string& user_id, const std::string& conversation_id, int delta) = 0;
    virtual bool ClearUnread(const std::string& user_id, const std::string& conversation_id) = 0;
};

/** RocksDB 会话存储实现 */
class RocksDBConversationStore : public ConversationStore {
public:
    explicit RocksDBConversationStore(const std::string& db_path);
    ~RocksDBConversationStore() override;

    bool Upsert(const std::string& user_id, const ConversationData& conv) override;
    std::vector<ConversationData> GetList(const std::string& user_id) override;
    bool Delete(const std::string& user_id, const std::string& conversation_id) override;
    bool UpdateUnread(const std::string& user_id, const std::string& conversation_id, int delta) override;
    bool ClearUnread(const std::string& user_id, const std::string& conversation_id) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace swift::chat
