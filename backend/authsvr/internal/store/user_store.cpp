/**
 * @file user_store.cpp
 * @brief RocksDB 用户存储实现
 *
 * Key 设计：
 *   user:{user_id}          -> UserData JSON
 *   username:{username}     -> user_id（用于登录时根据用户名查找）
 */

#include "user_store.h"
#include <nlohmann/json.hpp>
#include <rocksdb/db.h>
#include <rocksdb/write_batch.h>
#include <stdexcept>

using json = nlohmann::json;

namespace swift::auth {

// ============================================================================
// JSON 序列化/反序列化
// ============================================================================

namespace {

/// UserData -> JSON string
std::string SerializeUser(const UserData &user) {
  json j;
  j["user_id"] = user.user_id;
  j["username"] = user.username;
  j["password_hash"] = user.password_hash;
  j["nickname"] = user.nickname;
  j["avatar_url"] = user.avatar_url;
  j["signature"] = user.signature;
  j["gender"] = user.gender;
  j["created_at"] = user.created_at;
  j["updated_at"] = user.updated_at;
  return j.dump();
}

/// JSON string -> UserData
UserData DeserializeUser(const std::string &data) {
  json j = json::parse(data);
  UserData user;
  user.user_id = j.value("user_id", "");
  user.username = j.value("username", "");
  user.password_hash = j.value("password_hash", "");
  user.nickname = j.value("nickname", "");
  user.avatar_url = j.value("avatar_url", "");
  user.signature = j.value("signature", "");
  user.gender = j.value("gender", 0);
  user.created_at = j.value("created_at", 0);
  user.updated_at = j.value("updated_at", 0);
  return user;
}

/// Key 前缀
constexpr const char *KEY_PREFIX_USER = "user:";
constexpr const char *KEY_PREFIX_USERNAME = "username:";

} // namespace

// ============================================================================
// RocksDBUserStore 实现
// ============================================================================

struct RocksDBUserStore::Impl {
  rocksdb::DB *db = nullptr;
  std::string db_path;

  ~Impl() {
    if (db) {
      delete db;
      db = nullptr;
    }
  }
};

RocksDBUserStore::RocksDBUserStore(const std::string &db_path)
    : impl_(std::make_unique<Impl>()) {

  impl_->db_path = db_path;

  // 配置 RocksDB
  rocksdb::Options options;
  options.create_if_missing = true;
  options.IncreaseParallelism();
  options.OptimizeLevelStyleCompaction();

  // 打开数据库
  rocksdb::Status status = rocksdb::DB::Open(options, db_path, &impl_->db);
  if (!status.ok()) {
    throw std::runtime_error("Failed to open RocksDB: " + status.ToString());
  }
}

RocksDBUserStore::~RocksDBUserStore() = default;

bool RocksDBUserStore::Create(const UserData &user) {
  if (!impl_->db)
    return false;

  // 1. 检查用户名是否已存在
  if (UsernameExists(user.username)) {
    return false; // 用户名已存在
  }

  // 2. 序列化用户数据
  std::string value = SerializeUser(user);

  // 3. 使用 WriteBatch 原子写入两个 key
  rocksdb::WriteBatch batch;
  batch.Put(KEY_PREFIX_USER + user.user_id, value);
  batch.Put(KEY_PREFIX_USERNAME + user.username, user.user_id);

  rocksdb::WriteOptions write_opts;
  write_opts.sync = true; // 确保持久化

  rocksdb::Status status = impl_->db->Write(write_opts, &batch);
  return status.ok();
}

std::optional<UserData> RocksDBUserStore::GetById(const std::string &user_id) {
  if (!impl_->db || user_id.empty())
    return std::nullopt;

  std::string value;
  rocksdb::Status status =
      impl_->db->Get(rocksdb::ReadOptions(), KEY_PREFIX_USER + user_id, &value);

  if (!status.ok()) {
    return std::nullopt;
  }

  try {
    return DeserializeUser(value);
  } catch (const std::exception &e) {
    // JSON 解析失败
    return std::nullopt;
  }
}

std::optional<UserData>
RocksDBUserStore::GetByUsername(const std::string &username) {
  if (!impl_->db || username.empty())
    return std::nullopt;

  // 1. 根据 username 获取 user_id
  std::string user_id;
  rocksdb::Status status = impl_->db->Get(
      rocksdb::ReadOptions(), KEY_PREFIX_USERNAME + username, &user_id);

  if (!status.ok()) {
    return std::nullopt;
  }

  // 2. 根据 user_id 获取完整用户数据
  return GetById(user_id);
}

bool RocksDBUserStore::Update(const UserData &user) {
  if (!impl_->db || user.user_id.empty())
    return false;

  // 1. 检查用户是否存在
  auto existing = GetById(user.user_id);
  if (!existing) {
    return false; // 用户不存在
  }

  // 2. 如果用户名变更，需要更新索引
  rocksdb::WriteBatch batch;

  if (existing->username != user.username) {
    // 检查新用户名是否已被占用
    if (UsernameExists(user.username)) {
      return false;
    }
    // 删除旧的 username 索引
    batch.Delete(KEY_PREFIX_USERNAME + existing->username);
    // 添加新的 username 索引
    batch.Put(KEY_PREFIX_USERNAME + user.username, user.user_id);
  }

  // 3. 更新用户数据
  std::string value = SerializeUser(user);
  batch.Put(KEY_PREFIX_USER + user.user_id, value);

  rocksdb::WriteOptions write_opts;
  write_opts.sync = true;

  rocksdb::Status status = impl_->db->Write(write_opts, &batch);
  return status.ok();
}

bool RocksDBUserStore::UsernameExists(const std::string &username) {
  if (!impl_->db || username.empty())
    return false;

  std::string value;
  rocksdb::Status status = impl_->db->Get(
      rocksdb::ReadOptions(), KEY_PREFIX_USERNAME + username, &value);

  return status.ok();
}

} // namespace swift::auth
