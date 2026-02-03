/**
 * @file friend_store.cpp
 * @brief RocksDB 好友存储实现
 *
 * Key 设计：
 *   friend:{user_id}:{friend_id}        -> FriendData JSON
 *   friend_req:{request_id}             -> FriendRequestData JSON
 *   friend_req_to:{to_user_id}:{req_id}  -> "" (收到的请求索引)
 *   friend_req_from:{from_user_id}:{req_id} -> "" (发出的请求索引)
 *   friend_group:{user_id}:{group_id}   -> FriendGroupData JSON
 *   block:{user_id}:{target_id}         -> "1"
 */

#include "friend_store.h"
#include <nlohmann/json.hpp>
#include <rocksdb/db.h>
#include <rocksdb/iterator.h>
#include <rocksdb/write_batch.h>
#include <stdexcept>

using json = nlohmann::json;

namespace swift::friend_ {

namespace {

// ============== 序列化 ==============

std::string SerializeFriend(const FriendData &d) {
  json j;
  j["user_id"] = d.user_id;
  j["friend_id"] = d.friend_id;
  j["remark"] = d.remark;
  j["group_id"] = d.group_id;
  j["added_at"] = d.added_at;
  return j.dump();
}

FriendData DeserializeFriend(const std::string &data) {
  json j = json::parse(data);
  FriendData d;
  d.user_id = j.value("user_id", "");
  d.friend_id = j.value("friend_id", "");
  d.remark = j.value("remark", "");
  d.group_id = j.value("group_id", "");
  d.added_at = j.value("added_at", 0);
  return d;
}

std::string SerializeRequest(const FriendRequestData &r) {
  json j;
  j["request_id"] = r.request_id;
  j["from_user_id"] = r.from_user_id;
  j["to_user_id"] = r.to_user_id;
  j["remark"] = r.remark;
  j["status"] = r.status;
  j["created_at"] = r.created_at;
  return j.dump();
}

FriendRequestData DeserializeRequest(const std::string &data) {
  json j = json::parse(data);
  FriendRequestData r;
  r.request_id = j.value("request_id", "");
  r.from_user_id = j.value("from_user_id", "");
  r.to_user_id = j.value("to_user_id", "");
  r.remark = j.value("remark", "");
  r.status = j.value("status", 0);
  r.created_at = j.value("created_at", 0);
  return r;
}

std::string SerializeGroup(const FriendGroupData &g) {
  json j;
  j["group_id"] = g.group_id;
  j["user_id"] = g.user_id;
  j["group_name"] = g.group_name;
  j["sort_order"] = g.sort_order;
  return j.dump();
}

FriendGroupData DeserializeGroup(const std::string &data) {
  json j = json::parse(data);
  FriendGroupData g;
  g.group_id = j.value("group_id", "");
  g.user_id = j.value("user_id", "");
  g.group_name = j.value("group_name", "");
  g.sort_order = j.value("sort_order", 0);
  return g;
}

// ============== Key 前缀 ==============

constexpr const char *K_FRIEND = "friend:";
constexpr const char *K_FRIEND_REQ = "friend_req:";
constexpr const char *K_FRIEND_REQ_TO = "friend_req_to:";
constexpr const char *K_FRIEND_REQ_FROM = "friend_req_from:";
constexpr const char *K_FRIEND_GROUP = "friend_group:";
constexpr const char *K_BLOCK = "block:";

std::string KeyFriend(const std::string &user_id,
                      const std::string &friend_id) {
  return std::string(K_FRIEND) + user_id + ":" + friend_id;
}
std::string KeyFriendReq(const std::string &request_id) {
  return std::string(K_FRIEND_REQ) + request_id;
}
std::string KeyFriendReqTo(const std::string &to_user_id,
                           const std::string &request_id) {
  return std::string(K_FRIEND_REQ_TO) + to_user_id + ":" + request_id;
}
std::string KeyFriendReqFrom(const std::string &from_user_id,
                             const std::string &request_id) {
  return std::string(K_FRIEND_REQ_FROM) + from_user_id + ":" + request_id;
}
std::string KeyFriendGroup(const std::string &user_id,
                           const std::string &group_id) {
  return std::string(K_FRIEND_GROUP) + user_id + ":" + group_id;
}
std::string KeyBlock(const std::string &user_id, const std::string &target_id) {
  return std::string(K_BLOCK) + user_id + ":" + target_id;
}

std::string PrefixFriend(const std::string &user_id) {
  return std::string(K_FRIEND) + user_id + ":";
}
std::string PrefixFriendReqTo(const std::string &to_user_id) {
  return std::string(K_FRIEND_REQ_TO) + to_user_id + ":";
}
std::string PrefixFriendReqFrom(const std::string &from_user_id) {
  return std::string(K_FRIEND_REQ_FROM) + from_user_id + ":";
}
std::string PrefixFriendGroup(const std::string &user_id) {
  return std::string(K_FRIEND_GROUP) + user_id + ":";
}
std::string PrefixBlock(const std::string &user_id) {
  return std::string(K_BLOCK) + user_id + ":";
}

} // namespace

// ============================================================================
// Impl
// ============================================================================

struct RocksDBFriendStore::Impl {
  rocksdb::DB *db = nullptr;
  std::string db_path;

  ~Impl() {
    if (db) {
      delete db;
      db = nullptr;
    }
  }
};

RocksDBFriendStore::RocksDBFriendStore(const std::string &db_path)
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

RocksDBFriendStore::~RocksDBFriendStore() = default;

// ============================================================================
// 好友关系
// ============================================================================

bool RocksDBFriendStore::AddFriend(const FriendData &data) {
  if (!impl_->db || data.user_id.empty() || data.friend_id.empty())
    return false;
  if (IsFriend(data.user_id, data.friend_id))
    return false;

  // 双向关系：A->B 与 B->A
  FriendData reverse;
  reverse.user_id = data.friend_id;
  reverse.friend_id = data.user_id;
  reverse.remark = "";
  reverse.group_id = kDefaultFriendGroupId;
  reverse.added_at = data.added_at;

  rocksdb::WriteBatch batch;
  batch.Put(KeyFriend(data.user_id, data.friend_id), SerializeFriend(data));
  batch.Put(KeyFriend(reverse.user_id, reverse.friend_id),
            SerializeFriend(reverse));

  rocksdb::WriteOptions wo;
  wo.sync = true;
  return impl_->db->Write(wo, &batch).ok();
}

bool RocksDBFriendStore::RemoveFriend(const std::string &user_id,
                                      const std::string &friend_id) {
  if (!impl_->db || user_id.empty() || friend_id.empty())
    return false;

  rocksdb::WriteBatch batch;
  batch.Delete(KeyFriend(user_id, friend_id));
  batch.Delete(KeyFriend(friend_id, user_id));

  rocksdb::WriteOptions wo;
  wo.sync = true;
  return impl_->db->Write(wo, &batch).ok();
}

std::vector<FriendData>
RocksDBFriendStore::GetFriends(const std::string &user_id,
                               const std::string &group_id) {
  std::vector<FriendData> result;
  if (!impl_->db || user_id.empty())
    return result;

  std::string prefix = PrefixFriend(user_id);
  rocksdb::Slice prefix_slice(prefix);
  std::unique_ptr<rocksdb::Iterator> it(
      impl_->db->NewIterator(rocksdb::ReadOptions()));
  for (it->Seek(prefix); it->Valid(); it->Next()) {
    if (!it->key().starts_with(prefix_slice))
      break;
    try {
      FriendData d = DeserializeFriend(it->value().ToString());
      // 分组 id 为空视为非法好友，不返回
      if (d.group_id.empty())
        continue;
      if (group_id.empty() || d.group_id == group_id)
        result.push_back(std::move(d));
    } catch (...) {
      /* skip bad entry */
    }
  }
  return result;
}

bool RocksDBFriendStore::IsFriend(const std::string &user_id,
                                  const std::string &friend_id) {
  if (!impl_->db || user_id.empty() || friend_id.empty())
    return false;
  std::string value;
  return impl_->db
      ->Get(rocksdb::ReadOptions(), KeyFriend(user_id, friend_id), &value)
      .ok();
}

bool RocksDBFriendStore::UpdateRemark(const std::string &user_id,
                                      const std::string &friend_id,
                                      const std::string &remark) {
  if (!impl_->db || user_id.empty() || friend_id.empty())
    return false;

  std::string value;
  if (!impl_->db
           ->Get(rocksdb::ReadOptions(), KeyFriend(user_id, friend_id), &value)
           .ok())
    return false;

  FriendData d = DeserializeFriend(value);
  d.remark = remark;
  rocksdb::WriteOptions wo;
  wo.sync = true;
  return impl_->db->Put(wo, KeyFriend(user_id, friend_id), SerializeFriend(d))
      .ok();
}

bool RocksDBFriendStore::MoveFriend(const std::string &user_id,
                                    const std::string &friend_id,
                                    const std::string &to_group_id) {
  if (!impl_->db || user_id.empty() || friend_id.empty())
    return false;

  std::string value;
  if (!impl_->db
           ->Get(rocksdb::ReadOptions(), KeyFriend(user_id, friend_id), &value)
           .ok())
    return false;

  FriendData d = DeserializeFriend(value);
  d.group_id = to_group_id;
  rocksdb::WriteOptions wo;
  wo.sync = true;
  return impl_->db->Put(wo, KeyFriend(user_id, friend_id), SerializeFriend(d))
      .ok();
}

// ============================================================================
// 好友请求
// ============================================================================

bool RocksDBFriendStore::CreateRequest(const FriendRequestData &req) {
  if (!impl_->db || req.request_id.empty() || req.from_user_id.empty() ||
      req.to_user_id.empty())
    return false;

  std::string req_key = KeyFriendReq(req.request_id);
  std::string value;
  if (impl_->db->Get(rocksdb::ReadOptions(), req_key, &value).ok())
    return false; // request_id 已存在

  rocksdb::WriteBatch batch;
  batch.Put(req_key, SerializeRequest(req));
  batch.Put(KeyFriendReqTo(req.to_user_id, req.request_id), "");
  batch.Put(KeyFriendReqFrom(req.from_user_id, req.request_id), "");

  rocksdb::WriteOptions wo;
  wo.sync = true;
  return impl_->db->Write(wo, &batch).ok();
}

std::optional<FriendRequestData>
RocksDBFriendStore::GetRequest(const std::string &request_id) {
  if (!impl_->db || request_id.empty())
    return std::nullopt;

  std::string value;
  if (!impl_->db->Get(rocksdb::ReadOptions(), KeyFriendReq(request_id), &value)
           .ok())
    return std::nullopt;

  try {
    return DeserializeRequest(value);
  } catch (...) {
    return std::nullopt;
  }
}

bool RocksDBFriendStore::UpdateRequestStatus(const std::string &request_id,
                                             int status) {
  if (!impl_->db || request_id.empty())
    return false;

  auto req = GetRequest(request_id);
  if (!req)
    return false;

  req->status = status;
  rocksdb::WriteOptions wo;
  wo.sync = true;
  return impl_->db->Put(wo, KeyFriendReq(request_id), SerializeRequest(*req))
      .ok();
}

std::vector<FriendRequestData>
RocksDBFriendStore::GetReceivedRequests(const std::string &user_id) {
  std::vector<FriendRequestData> result;
  if (!impl_->db || user_id.empty())
    return result;

  std::string prefix = PrefixFriendReqTo(user_id);
  rocksdb::Slice prefix_slice(prefix);
  std::unique_ptr<rocksdb::Iterator> it(
      impl_->db->NewIterator(rocksdb::ReadOptions()));
  for (it->Seek(prefix); it->Valid(); it->Next()) {
    if (!it->key().starts_with(prefix_slice))
      break;
    // key = friend_req_to:{to_user_id}:{request_id}
    std::string key = it->key().ToString();
    size_t pos = prefix.size();
    if (pos >= key.size())
      continue;
    std::string request_id = key.substr(pos);
    auto req = GetRequest(request_id);
    if (req)
      result.push_back(std::move(*req));
  }
  return result;
}

std::vector<FriendRequestData>
RocksDBFriendStore::GetSentRequests(const std::string &user_id) {
  std::vector<FriendRequestData> result;
  if (!impl_->db || user_id.empty())
    return result;

  std::string prefix = PrefixFriendReqFrom(user_id);
  rocksdb::Slice prefix_slice(prefix);
  std::unique_ptr<rocksdb::Iterator> it(
      impl_->db->NewIterator(rocksdb::ReadOptions()));
  for (it->Seek(prefix); it->Valid(); it->Next()) {
    if (!it->key().starts_with(prefix_slice))
      break;
    std::string key = it->key().ToString();
    size_t pos = prefix.size();
    if (pos >= key.size())
      continue;
    std::string request_id = key.substr(pos);
    auto req = GetRequest(request_id);
    if (req)
      result.push_back(std::move(*req));
  }
  return result;
}

// ============================================================================
// 分组
// ============================================================================

bool RocksDBFriendStore::CreateGroup(const FriendGroupData &group) {
  if (!impl_->db || group.user_id.empty() || group.group_id.empty())
    return false;

  std::string key = KeyFriendGroup(group.user_id, group.group_id);
  std::string value;
  if (impl_->db->Get(rocksdb::ReadOptions(), key, &value).ok())
    return false; // 分组已存在

  rocksdb::WriteOptions wo;
  wo.sync = true;
  return impl_->db->Put(wo, key, SerializeGroup(group)).ok();
}

std::vector<FriendGroupData>
RocksDBFriendStore::GetGroups(const std::string &user_id) {
  std::vector<FriendGroupData> result;
  if (!impl_->db || user_id.empty())
    return result;

  std::string prefix = PrefixFriendGroup(user_id);
  rocksdb::Slice prefix_slice(prefix);
  std::unique_ptr<rocksdb::Iterator> it(
      impl_->db->NewIterator(rocksdb::ReadOptions()));
  for (it->Seek(prefix); it->Valid(); it->Next()) {
    if (!it->key().starts_with(prefix_slice))
      break;
    try {
      result.push_back(DeserializeGroup(it->value().ToString()));
    } catch (...) {
      /* skip */
    }
  }
  return result;
}

bool RocksDBFriendStore::DeleteGroup(const std::string &user_id,
                                     const std::string &group_id) {
  if (!impl_->db || user_id.empty() || group_id.empty())
    return false;

  // 将该分组下所有好友移至默认分组
  std::vector<FriendData> friends = GetFriends(user_id, group_id);
  rocksdb::WriteBatch batch;
  batch.Delete(KeyFriendGroup(user_id, group_id));
  for (const auto &f : friends) {
    FriendData updated = f;
    updated.group_id = kDefaultFriendGroupId;
    batch.Put(KeyFriend(user_id, f.friend_id), SerializeFriend(updated));
  }

  rocksdb::WriteOptions wo;
  wo.sync = true;
  return impl_->db->Write(wo, &batch).ok();
}

// ============================================================================
// 黑名单
// ============================================================================

bool RocksDBFriendStore::Block(const std::string &user_id,
                               const std::string &target_id) {
  if (!impl_->db || user_id.empty() || target_id.empty())
    return false;

  rocksdb::WriteOptions wo;
  wo.sync = true;
  return impl_->db->Put(wo, KeyBlock(user_id, target_id), "1").ok();
}

bool RocksDBFriendStore::Unblock(const std::string &user_id,
                                 const std::string &target_id) {
  if (!impl_->db || user_id.empty() || target_id.empty())
    return false;

  rocksdb::WriteOptions wo;
  wo.sync = true;
  return impl_->db->Delete(wo, KeyBlock(user_id, target_id)).ok();
}

bool RocksDBFriendStore::IsBlocked(const std::string &user_id,
                                   const std::string &target_id) {
  if (!impl_->db || user_id.empty() || target_id.empty())
    return false;

  std::string value;
  return impl_->db
      ->Get(rocksdb::ReadOptions(), KeyBlock(user_id, target_id), &value)
      .ok();
}

std::vector<std::string>
RocksDBFriendStore::GetBlockList(const std::string &user_id) {
  std::vector<std::string> result;
  if (!impl_->db || user_id.empty())
    return result;

  std::string prefix = PrefixBlock(user_id);
  rocksdb::Slice prefix_slice(prefix);
  std::unique_ptr<rocksdb::Iterator> it(
      impl_->db->NewIterator(rocksdb::ReadOptions()));
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

} // namespace swift::friend_
