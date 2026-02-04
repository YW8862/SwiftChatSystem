/**
 * @file message_store.cpp
 * @brief RocksDB 消息与会话存储实现
 *
 * Key 设计：
 *   msg:{msg_id}                              -> MessageData JSON
 *   chat:{conversation_id}:{rev_ts}:{msg_id}  -> "" (会话时间线，rev_ts=MAX_TS-ts 便于倒序)
 *   offline:{user_id}:{rev_ts}:{msg_id}       -> "" (离线队列，按时间倒序)
 *   conv:{user_id}:{conversation_id}         -> ConversationData JSON
 *   conv_meta:{conversation_id}               -> 私聊会话元信息（ConversationRegistry）
 *
 * 撤回：仅更新消息 status=1、recall_at，不删除；服务器仍保留该消息。
 */

#include "message_store.h"
#include <nlohmann/json.hpp>
#include <rocksdb/db.h>
#include <rocksdb/iterator.h>
#include <rocksdb/write_batch.h>
#include <stdexcept>
#include <algorithm>
#include <cinttypes>
#include <cstdint>

using json = nlohmann::json;

namespace swift::chat {

namespace {

constexpr int64_t MAX_TS = 9999999999999;  // 用于 rev_ts = MAX_TS - timestamp，使键按时间倒序

// ============== 序列化 ==============

std::string SerializeMessage(const MessageData& m) {
    json j;
    j["msg_id"] = m.msg_id;
    j["from_user_id"] = m.from_user_id;
    j["to_id"] = m.to_id;
    j["conversation_id"] = m.conversation_id;
    j["chat_type"] = m.chat_type;
    j["content"] = m.content;
    j["media_url"] = m.media_url;
    j["media_type"] = m.media_type;
    j["mentions"] = m.mentions;
    j["reply_to_msg_id"] = m.reply_to_msg_id;
    j["timestamp"] = m.timestamp;
    j["status"] = m.status;
    j["recall_at"] = m.recall_at;
    return j.dump();
}

MessageData DeserializeMessage(const std::string& data) {
    json j = json::parse(data);
    MessageData m;
    m.msg_id = j.value("msg_id", "");
    m.from_user_id = j.value("from_user_id", "");
    m.to_id = j.value("to_id", "");
    m.conversation_id = j.value("conversation_id", j.value("chat_id", ""));
    m.chat_type = j.value("chat_type", 1);
    m.content = j.value("content", "");
    m.media_url = j.value("media_url", "");
    m.media_type = j.value("media_type", "");
    if (j.contains("mentions") && j["mentions"].is_array()) {
        for (const auto& x : j["mentions"])
            m.mentions.push_back(x.get<std::string>());
    }
    m.reply_to_msg_id = j.value("reply_to_msg_id", "");
    m.timestamp = j.value("timestamp", static_cast<int64_t>(0));
    m.status = j.value("status", 0);
    m.recall_at = j.value("recall_at", static_cast<int64_t>(0));
    return m;
}

std::string SerializeConversation(const ConversationData& c) {
    json j;
    j["conversation_id"] = c.conversation_id;
    j["chat_type"] = c.chat_type;
    j["peer_id"] = c.peer_id;
    j["last_msg_id"] = c.last_msg_id;
    j["unread_count"] = c.unread_count;
    j["updated_at"] = c.updated_at;
    j["is_pinned"] = c.is_pinned;
    j["is_muted"] = c.is_muted;
    return j.dump();
}

ConversationData DeserializeConversation(const std::string& data) {
    json j = json::parse(data);
    ConversationData c;
    c.conversation_id = j.value("conversation_id", j.value("chat_id", ""));
    c.chat_type = j.value("chat_type", 1);
    c.peer_id = j.value("peer_id", "");
    c.last_msg_id = j.value("last_msg_id", "");
    c.unread_count = j.value("unread_count", 0);
    c.updated_at = j.value("updated_at", static_cast<int64_t>(0));
    c.is_pinned = j.value("is_pinned", false);
    c.is_muted = j.value("is_muted", false);
    return c;
}

// ============== Key ==============

constexpr const char* K_MSG = "msg:";
constexpr const char* K_CHAT = "chat:";
constexpr const char* K_OFFLINE = "offline:";
constexpr const char* K_CONV = "conv:";
constexpr const char* K_CONV_META = "conv_meta:";

std::string KeyMsg(const std::string& msg_id) {
    return std::string(K_MSG) + msg_id;
}

// rev_ts 固定 13 位，便于字典序
std::string RevTs(int64_t timestamp) {
    int64_t rev = (timestamp <= 0 || timestamp > MAX_TS) ? MAX_TS : (MAX_TS - timestamp);
    char buf[16];
    snprintf(buf, sizeof(buf), "%013" PRId64, rev);
    return std::string(buf);
}

std::string KeyChat(const std::string& chat_id, int64_t timestamp, const std::string& msg_id) {
    return std::string(K_CHAT) + chat_id + ":" + RevTs(timestamp) + ":" + msg_id;
}

std::string PrefixChat(const std::string& chat_id) {
    return std::string(K_CHAT) + chat_id + ":";
}

std::string KeyOffline(const std::string& user_id, int64_t timestamp, const std::string& msg_id) {
    return std::string(K_OFFLINE) + user_id + ":" + RevTs(timestamp) + ":" + msg_id;
}

std::string PrefixOffline(const std::string& user_id) {
    return std::string(K_OFFLINE) + user_id + ":";
}

std::string KeyConv(const std::string& user_id, const std::string& conversation_id) {
    return std::string(K_CONV) + user_id + ":" + conversation_id;
}

std::string KeyConvMeta(const std::string& conversation_id) {
    return std::string(K_CONV_META) + conversation_id;
}

std::string PrefixConv(const std::string& user_id) {
    return std::string(K_CONV) + user_id + ":";
}

// 从 chat key 解析出 rev_ts 和 msg_id: chat:{chat_id}:{rev_ts}:{msg_id}
void ParseChatKey(const std::string& key, const std::string& prefix,
                  std::string& rev_ts, std::string& msg_id) {
    size_t pos = prefix.size();
    if (key.size() <= pos + 13 + 1) return;
    rev_ts = key.substr(pos, 13);
    msg_id = key.substr(pos + 14);
}

void ParseOfflineKey(const std::string& key, const std::string& prefix,
                     std::string& rev_ts, std::string& msg_id) {
    size_t pos = prefix.size();
    if (key.size() <= pos + 13 + 1) return;
    rev_ts = key.substr(pos, 13);
    msg_id = key.substr(pos + 14);
}

}  // namespace

// ============================================================================
// RocksDBMessageStore
// ============================================================================

struct RocksDBMessageStore::Impl {
    rocksdb::DB* db = nullptr;
    std::string db_path;

    ~Impl() {
        if (db) {
            delete db;
            db = nullptr;
        }
    }
};

RocksDBMessageStore::RocksDBMessageStore(const std::string& db_path)
    : impl_(std::make_unique<Impl>()) {
    impl_->db_path = db_path;
    rocksdb::Options options;
    options.create_if_missing = true;
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    rocksdb::Status status = rocksdb::DB::Open(options, db_path, &impl_->db);
    if (!status.ok()) {
        throw std::runtime_error("Failed to open RocksDB (message): " + status.ToString());
    }
}

RocksDBMessageStore::~RocksDBMessageStore() = default;

bool RocksDBMessageStore::Save(const MessageData& msg) {
    if (!impl_->db || msg.msg_id.empty() || msg.from_user_id.empty())
        return false;

    std::string value;
    if (impl_->db->Get(rocksdb::ReadOptions(), KeyMsg(msg.msg_id), &value).ok())
        return false;  // 已存在，不允许覆盖（或可改为 Upsert，按需求）

    rocksdb::WriteBatch batch;
    batch.Put(KeyMsg(msg.msg_id), SerializeMessage(msg));
    std::string timeline_id = msg.conversation_id.empty() ? msg.to_id : msg.conversation_id;
    batch.Put(KeyChat(timeline_id, msg.timestamp, msg.msg_id), "");
    rocksdb::WriteOptions wo;
    wo.sync = true;
    return impl_->db->Write(wo, &batch).ok();
}

std::optional<MessageData> RocksDBMessageStore::GetById(const std::string& msg_id) {
    if (!impl_->db || msg_id.empty())
        return std::nullopt;

    std::string value;
    if (!impl_->db->Get(rocksdb::ReadOptions(), KeyMsg(msg_id), &value).ok())
        return std::nullopt;

    try {
        return DeserializeMessage(value);
    } catch (...) {
        return std::nullopt;
    }
}

std::vector<MessageData> RocksDBMessageStore::GetHistory(const std::string& conversation_id,
                                                          int /*chat_type*/,
                                                          const std::string& before_msg_id,
                                                          int limit) {
    std::vector<MessageData> result;
    if (!impl_->db || conversation_id.empty() || limit <= 0)
        return result;

    std::string prefix = PrefixChat(conversation_id);
    rocksdb::Slice prefix_slice(prefix);
    std::unique_ptr<rocksdb::Iterator> it(impl_->db->NewIterator(rocksdb::ReadOptions()));

    int64_t rev_ts_cutoff = -1;  // -1 表示不按 before 过滤，取最新 limit 条
    if (!before_msg_id.empty()) {
        auto msg = GetById(before_msg_id);
        if (!msg || (msg->conversation_id != conversation_id && msg->to_id != conversation_id))
            return result;
        rev_ts_cutoff = MAX_TS - msg->timestamp;  // 只取比 before 更旧的（rev_ts > rev_ts_cutoff）
    }

    std::vector<std::pair<std::string, std::string>> keys;  // rev_ts, msg_id
    for (it->Seek(prefix); it->Valid(); it->Next()) {
        if (!it->key().starts_with(prefix_slice))
            break;
        std::string key = it->key().ToString();
        std::string rev_ts, msg_id;
        ParseChatKey(key, prefix, rev_ts, msg_id);
        if (rev_ts.size() != 13) continue;
        int64_t r = 0;
        try { r = std::stoll(rev_ts); } catch (...) { continue; }
        if (rev_ts_cutoff >= 0 && r <= rev_ts_cutoff) continue;  // 只取比 before 更旧的（r 更大 = 时间更早）
        keys.emplace_back(rev_ts, msg_id);
        if (static_cast<int>(keys.size()) >= limit)
            break;
    }

    for (const auto& p : keys) {
        auto msg = GetById(p.second);
        if (msg)
            result.push_back(std::move(*msg));
    }
    return result;
}

bool RocksDBMessageStore::MarkRecalled(const std::string& msg_id, int64_t recall_at) {
    if (!impl_->db || msg_id.empty())
        return false;

    auto msg = GetById(msg_id);
    if (!msg)
        return false;

    msg->status = 1;
    msg->recall_at = recall_at;
    rocksdb::WriteOptions wo;
    wo.sync = true;
    return impl_->db->Put(wo, KeyMsg(msg_id), SerializeMessage(*msg)).ok();
}

bool RocksDBMessageStore::AddToOffline(const std::string& user_id,
                                       const std::string& msg_id) {
    if (!impl_->db || user_id.empty() || msg_id.empty())
        return false;

    auto msg = GetById(msg_id);
    if (!msg)
        return false;

    std::string key = KeyOffline(user_id, msg->timestamp, msg_id);
    rocksdb::WriteOptions wo;
    wo.sync = true;
    return impl_->db->Put(wo, key, "").ok();
}

std::vector<MessageData> RocksDBMessageStore::PullOffline(const std::string& user_id,
                                                          const std::string& cursor,
                                                          int limit,
                                                          std::string& next_cursor,
                                                          bool& has_more) {
    std::vector<MessageData> result;
    next_cursor.clear();
    has_more = false;
    if (!impl_->db || user_id.empty() || limit <= 0)
        return result;

    std::string prefix = PrefixOffline(user_id);
    rocksdb::Slice prefix_slice(prefix);
    std::unique_ptr<rocksdb::Iterator> it(impl_->db->NewIterator(rocksdb::ReadOptions()));

    bool skip = !cursor.empty();
    int count = 0;
    std::string last_rev_ts;
    for (it->Seek(prefix); it->Valid(); it->Next()) {
        if (!it->key().starts_with(prefix_slice))
            break;
        std::string key = it->key().ToString();
        std::string rev_ts, msg_id;
        ParseOfflineKey(key, prefix, rev_ts, msg_id);
        if (skip && rev_ts == cursor) { skip = false; continue; }
        if (skip) continue;

        auto msg = GetById(msg_id);
        if (msg) {
            result.push_back(std::move(*msg));
            last_rev_ts = rev_ts;
            if (++count >= limit) break;
        }
    }

    if (count >= limit && it->Valid() && it->key().starts_with(prefix_slice)) {
        it->Next();
        if (it->Valid() && it->key().starts_with(prefix_slice)) {
            next_cursor = last_rev_ts;
            has_more = true;
        }
    }
    return result;
}

bool RocksDBMessageStore::ClearOffline(const std::string& user_id,
                                       const std::string& until_msg_id) {
    if (!impl_->db || user_id.empty())
        return true;
    if (until_msg_id.empty()) {
        std::string prefix = PrefixOffline(user_id);
        rocksdb::Slice prefix_slice(prefix);
        std::unique_ptr<rocksdb::Iterator> it(impl_->db->NewIterator(rocksdb::ReadOptions()));
        rocksdb::WriteBatch batch;
        for (it->Seek(prefix); it->Valid(); it->Next()) {
            if (!it->key().starts_with(prefix_slice)) break;
            batch.Delete(it->key().ToString());
        }
        rocksdb::WriteOptions wo;
        wo.sync = true;
        return impl_->db->Write(wo, &batch).ok();
    }

    auto until_msg = GetById(until_msg_id);
    if (!until_msg)
        return true;
    int64_t until_ts = until_msg->timestamp;
    std::string prefix = PrefixOffline(user_id);
    rocksdb::Slice prefix_slice(prefix);
    std::unique_ptr<rocksdb::Iterator> it(impl_->db->NewIterator(rocksdb::ReadOptions()));
    rocksdb::WriteBatch batch;
    for (it->Seek(prefix); it->Valid(); it->Next()) {
        if (!it->key().starts_with(prefix_slice)) break;
        std::string key = it->key().ToString();
        std::string rev_ts, msg_id;
        ParseOfflineKey(key, prefix, rev_ts, msg_id);
        int64_t r = 0;
        try { r = std::stoll(rev_ts); } catch (...) { continue; }
        int64_t ts = MAX_TS - r;
        if (ts <= until_ts)
            batch.Delete(key);
    }
    rocksdb::WriteOptions wo;
    wo.sync = true;
    return impl_->db->Write(wo, &batch).ok();
}

// ============================================================================
// RocksDBConversationStore
// ============================================================================

struct RocksDBConversationStore::Impl {
    rocksdb::DB* db = nullptr;
    std::string db_path;

    ~Impl() {
        if (db) {
            delete db;
            db = nullptr;
        }
    }
};

RocksDBConversationStore::RocksDBConversationStore(const std::string& db_path)
    : impl_(std::make_unique<Impl>()) {
    impl_->db_path = db_path;
    rocksdb::Options options;
    options.create_if_missing = true;
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    rocksdb::Status status = rocksdb::DB::Open(options, db_path, &impl_->db);
    if (!status.ok()) {
        throw std::runtime_error("Failed to open RocksDB (conversation): " + status.ToString());
    }
}

RocksDBConversationStore::~RocksDBConversationStore() = default;

bool RocksDBConversationStore::Upsert(const std::string& user_id, const ConversationData& conv) {
    if (!impl_->db || user_id.empty() || conv.conversation_id.empty())
        return false;

    std::string key = KeyConv(user_id, conv.conversation_id);
    rocksdb::WriteOptions wo;
    wo.sync = true;
    return impl_->db->Put(wo, key, SerializeConversation(conv)).ok();
}

std::vector<ConversationData> RocksDBConversationStore::GetList(const std::string& user_id) {
    std::vector<ConversationData> result;
    if (!impl_->db || user_id.empty())
        return result;

    std::string prefix = PrefixConv(user_id);
    rocksdb::Slice prefix_slice(prefix);
    std::unique_ptr<rocksdb::Iterator> it(impl_->db->NewIterator(rocksdb::ReadOptions()));
    for (it->Seek(prefix); it->Valid(); it->Next()) {
        if (!it->key().starts_with(prefix_slice))
            break;
        try {
            result.push_back(DeserializeConversation(it->value().ToString()));
        } catch (...) {}
    }
    return result;
}

bool RocksDBConversationStore::Delete(const std::string& user_id, const std::string& conversation_id) {
    if (!impl_->db || user_id.empty() || conversation_id.empty())
        return false;

    rocksdb::WriteOptions wo;
    wo.sync = true;
    return impl_->db->Delete(wo, KeyConv(user_id, conversation_id)).ok();
}

bool RocksDBConversationStore::UpdateUnread(const std::string& user_id,
                                            const std::string& conversation_id, int delta) {
    if (!impl_->db || user_id.empty() || conversation_id.empty())
        return false;

    std::string key = KeyConv(user_id, conversation_id);
    std::string value;
    if (!impl_->db->Get(rocksdb::ReadOptions(), key, &value).ok())
        return false;

    try {
        ConversationData c = DeserializeConversation(value);
        c.unread_count = std::max(0, c.unread_count + delta);
        rocksdb::WriteOptions wo;
        wo.sync = true;
        return impl_->db->Put(wo, key, SerializeConversation(c)).ok();
    } catch (...) {
        return false;
    }
}

bool RocksDBConversationStore::ClearUnread(const std::string& user_id,
                                           const std::string& conversation_id) {
    if (!impl_->db || user_id.empty() || conversation_id.empty())
        return false;

    std::string key = KeyConv(user_id, conversation_id);
    std::string value;
    if (!impl_->db->Get(rocksdb::ReadOptions(), key, &value).ok())
        return false;

    try {
        ConversationData c = DeserializeConversation(value);
        c.unread_count = 0;
        rocksdb::WriteOptions wo;
        wo.sync = true;
        return impl_->db->Put(wo, key, SerializeConversation(c)).ok();
    } catch (...) {
        return false;
    }
}

// ============================================================================
// RocksDBConversationRegistry
// ============================================================================

struct RocksDBConversationRegistry::Impl {
    rocksdb::DB* db = nullptr;
    std::string db_path;

    ~Impl() {
        if (db) {
            delete db;
            db = nullptr;
        }
    }
};

RocksDBConversationRegistry::RocksDBConversationRegistry(const std::string& db_path)
    : impl_(std::make_unique<Impl>()) {
    impl_->db_path = db_path;
    rocksdb::Options options;
    options.create_if_missing = true;
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    rocksdb::Status status = rocksdb::DB::Open(options, db_path, &impl_->db);
    if (!status.ok()) {
        throw std::runtime_error("Failed to open RocksDB (conv_registry): " + status.ToString());
    }
}

RocksDBConversationRegistry::~RocksDBConversationRegistry() = default;

std::string RocksDBConversationRegistry::GetOrCreatePrivateConversation(
    const std::string& user_id_1, const std::string& user_id_2) {
    if (!impl_->db || user_id_1.empty() || user_id_2.empty())
        return "";
    std::string a = user_id_1, b = user_id_2;
    if (a > b) std::swap(a, b);
    std::string conversation_id = "p_" + a + "_" + b;
    std::string key = KeyConvMeta(conversation_id);
    std::string value;
    if (!impl_->db->Get(rocksdb::ReadOptions(), key, &value).ok()) {
        json j;
        j["type"] = "private";
        rocksdb::WriteOptions wo;
        wo.sync = true;
        impl_->db->Put(wo, key, j.dump());
    }
    return conversation_id;
}

}  // namespace swift::chat
