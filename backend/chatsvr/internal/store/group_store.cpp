/**
 * @file group_store.cpp
 * @brief RocksDB 群组存储实现
 *
 * Key 设计：
 *   group:{group_id}                 -> GroupData JSON
 *   group_member:{group_id}:{user_id} -> GroupMemberData JSON
 *   user_groups:{user_id}:{group_id}  -> ""
 */

#include "group_store.h"
#include <nlohmann/json.hpp>
#include <rocksdb/db.h>
#include <rocksdb/write_batch.h>
#include <rocksdb/iterator.h>
#include <stdexcept>
#include <algorithm>

using json = nlohmann::json;

namespace swift::group_ {

namespace {

// ============== 序列化 ==============

std::string SerializeGroup(const GroupData& g) {
    json j;
    j["group_id"] = g.group_id;
    j["group_name"] = g.group_name;
    j["avatar_url"] = g.avatar_url;
    j["owner_id"] = g.owner_id;
    j["member_count"] = g.member_count;
    j["announcement"] = g.announcement;
    j["created_at"] = g.created_at;
    j["updated_at"] = g.updated_at;
    return j.dump();
}

GroupData DeserializeGroup(const std::string& data) {
    json j = json::parse(data);
    GroupData g;
    g.group_id = j.value("group_id", "");
    g.group_name = j.value("group_name", "");
    g.avatar_url = j.value("avatar_url", "");
    g.owner_id = j.value("owner_id", "");
    g.member_count = j.value("member_count", 0);
    g.announcement = j.value("announcement", "");
    g.created_at = j.value("created_at", static_cast<int64_t>(0));
    g.updated_at = j.value("updated_at", static_cast<int64_t>(0));
    return g;
}

std::string SerializeMember(const GroupMemberData& m) {
    json j;
    j["user_id"] = m.user_id;
    j["role"] = m.role;
    j["nickname"] = m.nickname;
    j["joined_at"] = m.joined_at;
    return j.dump();
}

GroupMemberData DeserializeMember(const std::string& data) {
    json j = json::parse(data);
    GroupMemberData m;
    m.user_id = j.value("user_id", "");
    m.role = j.value("role", 1);
    m.nickname = j.value("nickname", "");
    m.joined_at = j.value("joined_at", static_cast<int64_t>(0));
    return m;
}

// ============== Key 前缀 ==============

constexpr const char* K_GROUP = "group:";
constexpr const char* K_GROUP_MEMBER = "group_member:";
constexpr const char* K_USER_GROUPS = "user_groups:";

std::string KeyGroup(const std::string& group_id) {
    return std::string(K_GROUP) + group_id;
}
std::string KeyGroupMember(const std::string& group_id, const std::string& user_id) {
    return std::string(K_GROUP_MEMBER) + group_id + ":" + user_id;
}
std::string KeyUserGroup(const std::string& user_id, const std::string& group_id) {
    return std::string(K_USER_GROUPS) + user_id + ":" + group_id;
}
std::string PrefixGroupMember(const std::string& group_id) {
    return std::string(K_GROUP_MEMBER) + group_id + ":";
}
std::string PrefixUserGroups(const std::string& user_id) {
    return std::string(K_USER_GROUPS) + user_id + ":";
}

}  // namespace

// ============================================================================
// Impl
// ============================================================================

struct RocksDBGroupStore::Impl {
    rocksdb::DB* db = nullptr;
    std::string db_path;

    ~Impl() {
        if (db) {
            delete db;
            db = nullptr;
        }
    }
};

RocksDBGroupStore::RocksDBGroupStore(const std::string& db_path)
    : impl_(std::make_unique<Impl>()) {
    impl_->db_path = db_path;
    rocksdb::Options options;
    options.create_if_missing = true;
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    rocksdb::Status status = rocksdb::DB::Open(options, db_path, &impl_->db);
    if (!status.ok()) {
        throw std::runtime_error("Failed to open RocksDB: " + status.ToString());
    }
}

RocksDBGroupStore::~RocksDBGroupStore() = default;

// ============================================================================
// 群组
// ============================================================================

bool RocksDBGroupStore::CreateGroup(const GroupData& data) {
    if (!impl_->db || data.group_id.empty() || data.owner_id.empty())
        return false;

    std::string key = KeyGroup(data.group_id);
    std::string value;
    if (impl_->db->Get(rocksdb::ReadOptions(), key, &value).ok())
        return false;  // 群已存在

    rocksdb::WriteOptions wo;
    wo.sync = true;
    return impl_->db->Put(wo, key, SerializeGroup(data)).ok();
}

std::optional<GroupData> RocksDBGroupStore::GetGroup(const std::string& group_id) {
    if (!impl_->db || group_id.empty())
        return std::nullopt;

    std::string value;
    if (!impl_->db->Get(rocksdb::ReadOptions(), KeyGroup(group_id), &value).ok())
        return std::nullopt;

    try {
        return DeserializeGroup(value);
    } catch (...) {
        return std::nullopt;
    }
}

bool RocksDBGroupStore::UpdateGroup(const std::string& group_id, const std::string& group_name,
                                    const std::string& avatar_url, const std::string& announcement,
                                    int64_t updated_at) {
    if (!impl_->db || group_id.empty())
        return false;

    auto g = GetGroup(group_id);
    if (!g)
        return false;

    g->group_name = group_name.empty() ? g->group_name : group_name;
    g->avatar_url = avatar_url.empty() ? g->avatar_url : avatar_url;
    g->announcement = announcement;
    if (updated_at != 0)
        g->updated_at = updated_at;
    rocksdb::WriteOptions wo;
    wo.sync = true;
    return impl_->db->Put(wo, KeyGroup(group_id), SerializeGroup(*g)).ok();
}

bool RocksDBGroupStore::UpdateGroupOwner(const std::string& group_id,
                                          const std::string& new_owner_id) {
    if (!impl_->db || group_id.empty() || new_owner_id.empty())
        return false;

    auto g = GetGroup(group_id);
    if (!g)
        return false;
    g->owner_id = new_owner_id;
    rocksdb::WriteOptions wo;
    wo.sync = true;
    return impl_->db->Put(wo, KeyGroup(group_id), SerializeGroup(*g)).ok();
}

bool RocksDBGroupStore::DeleteGroup(const std::string& group_id) {
    if (!impl_->db || group_id.empty())
        return false;

    // 先收集所有成员，删除 user_groups 索引
    std::string prefix = PrefixGroupMember(group_id);
    rocksdb::Slice prefix_slice(prefix);
    std::unique_ptr<rocksdb::Iterator> it(impl_->db->NewIterator(rocksdb::ReadOptions()));
    std::vector<std::string> user_ids;
    for (it->Seek(prefix); it->Valid(); it->Next()) {
        if (!it->key().starts_with(prefix_slice))
            break;
        std::string key = it->key().ToString();
        size_t pos = prefix.size();
        if (pos < key.size())
            user_ids.push_back(key.substr(pos));
    }

    rocksdb::WriteBatch batch;
    batch.Delete(KeyGroup(group_id));
    for (const auto& uid : user_ids) {
        batch.Delete(KeyGroupMember(group_id, uid));
        batch.Delete(KeyUserGroup(uid, group_id));
    }

    rocksdb::WriteOptions wo;
    wo.sync = true;
    return impl_->db->Write(wo, &batch).ok();
}

// ============================================================================
// 成员
// ============================================================================

bool RocksDBGroupStore::AddMember(const std::string& group_id, const GroupMemberData& member) {
    if (!impl_->db || group_id.empty() || member.user_id.empty())
        return false;

    std::string mem_key = KeyGroupMember(group_id, member.user_id);
    std::string value;
    if (impl_->db->Get(rocksdb::ReadOptions(), mem_key, &value).ok())
        return false;  // 已是成员

    auto g = GetGroup(group_id);
    if (!g)
        return false;

    rocksdb::WriteBatch batch;
    batch.Put(mem_key, SerializeMember(member));
    batch.Put(KeyUserGroup(member.user_id, group_id), "");
    g->member_count++;
    g->updated_at = 0;  // 上层可再写回
    batch.Put(KeyGroup(group_id), SerializeGroup(*g));

    rocksdb::WriteOptions wo;
    wo.sync = true;
    return impl_->db->Write(wo, &batch).ok();
}

bool RocksDBGroupStore::RemoveMember(const std::string& group_id, const std::string& user_id) {
    if (!impl_->db || group_id.empty() || user_id.empty())
        return false;

    std::string mem_key = KeyGroupMember(group_id, user_id);
    std::string value;
    if (!impl_->db->Get(rocksdb::ReadOptions(), mem_key, &value).ok())
        return false;  // 不是成员

    auto g = GetGroup(group_id);
    if (!g)
        return false;

    rocksdb::WriteBatch batch;
    batch.Delete(mem_key);
    batch.Delete(KeyUserGroup(user_id, group_id));
    g->member_count = std::max(0, g->member_count - 1);
    batch.Put(KeyGroup(group_id), SerializeGroup(*g));

    rocksdb::WriteOptions wo;
    wo.sync = true;
    return impl_->db->Write(wo, &batch).ok();
}

std::optional<GroupMemberData> RocksDBGroupStore::GetMember(const std::string& group_id,
                                                             const std::string& user_id) {
    if (!impl_->db || group_id.empty() || user_id.empty())
        return std::nullopt;

    std::string value;
    if (!impl_->db->Get(rocksdb::ReadOptions(), KeyGroupMember(group_id, user_id), &value).ok())
        return std::nullopt;

    try {
        return DeserializeMember(value);
    } catch (...) {
        return std::nullopt;
    }
}

std::vector<GroupMemberData> RocksDBGroupStore::GetMembers(const std::string& group_id,
                                                           int page, int page_size,
                                                           int* total_out) {
    std::vector<GroupMemberData> all;
    if (!impl_->db || group_id.empty())
        return all;

    std::string prefix = PrefixGroupMember(group_id);
    rocksdb::Slice prefix_slice(prefix);
    std::unique_ptr<rocksdb::Iterator> it(impl_->db->NewIterator(rocksdb::ReadOptions()));
    for (it->Seek(prefix); it->Valid(); it->Next()) {
        if (!it->key().starts_with(prefix_slice))
            break;
        try {
            all.push_back(DeserializeMember(it->value().ToString()));
        } catch (...) {
            /* skip */
        }
    }

    if (total_out)
        *total_out = static_cast<int>(all.size());

    if (page <= 0) page = 1;
    if (page_size <= 0) page_size = 50;
    int offset = (page - 1) * page_size;
    if (offset >= static_cast<int>(all.size()))
        return {};
    int end = std::min(offset + page_size, static_cast<int>(all.size()));
    return std::vector<GroupMemberData>(all.begin() + offset, all.begin() + end);
}

bool RocksDBGroupStore::UpdateMemberRole(const std::string& group_id, const std::string& user_id,
                                         int32_t role) {
    if (!impl_->db || group_id.empty() || user_id.empty())
        return false;

    auto m = GetMember(group_id, user_id);
    if (!m)
        return false;

    m->role = role;
    rocksdb::WriteOptions wo;
    wo.sync = true;
    return impl_->db->Put(wo, KeyGroupMember(group_id, user_id), SerializeMember(*m)).ok();
}

bool RocksDBGroupStore::IsMember(const std::string& group_id, const std::string& user_id) {
    if (!impl_->db || group_id.empty() || user_id.empty())
        return false;
    std::string value;
    return impl_->db->Get(rocksdb::ReadOptions(), KeyGroupMember(group_id, user_id), &value).ok();
}

std::vector<std::string> RocksDBGroupStore::GetUserGroupIds(const std::string& user_id) {
    std::vector<std::string> result;
    if (!impl_->db || user_id.empty())
        return result;

    std::string prefix = PrefixUserGroups(user_id);
    rocksdb::Slice prefix_slice(prefix);
    std::unique_ptr<rocksdb::Iterator> it(impl_->db->NewIterator(rocksdb::ReadOptions()));
    for (it->Seek(prefix); it->Valid(); it->Next()) {
        if (!it->key().starts_with(prefix_slice))
            break;
        std::string key = it->key().ToString();
        size_t pos = prefix.size();
        if (pos < key.size())
            result.push_back(key.substr(pos));
    }
    return result;
}

}  // namespace swift::group_
