#include "session_store.h"

#include <nlohmann/json.hpp>
#include <rocksdb/db.h>
#include <stdexcept>

using json = nlohmann::json;

namespace swift::online {

namespace {

constexpr const char *kSessionPrefix = "session:";

std::string SerializeSession(const SessionData &session) {
  json j;
  j["user_id"] = session.user_id;
  j["device_id"] = session.device_id;
  j["token"] = session.token;
  j["login_time"] = session.login_time;
  j["expire_at"] = session.expire_at;
  return j.dump();
}

SessionData DeserializeSession(const std::string &data) {
  json j = json::parse(data);
  SessionData session;
  session.user_id = j.value("user_id", "");
  session.device_id = j.value("device_id", "");
  session.token = j.value("token", "");
  session.login_time = j.value("login_time", static_cast<int64_t>(0));
  session.expire_at = j.value("expire_at", static_cast<int64_t>(0));
  return session;
}

} // namespace

struct RocksDBSessionStore::Impl {
  rocksdb::DB *db = nullptr;
  explicit Impl(rocksdb::DB *ptr) : db(ptr) {}
};

RocksDBSessionStore::RocksDBSessionStore(const std::string &db_path)
    : impl_(nullptr) {
  rocksdb::Options options;
  options.create_if_missing = true;

  rocksdb::DB *db = nullptr;
  auto status = rocksdb::DB::Open(options, db_path, &db);
  if (!status.ok()) {
    throw std::runtime_error("Failed to open RocksDB session store: " +
                             status.ToString());
  }
  impl_ = std::make_unique<Impl>(db);
}

RocksDBSessionStore::~RocksDBSessionStore() {
  if (impl_ && impl_->db) {
    delete impl_->db;
    impl_->db = nullptr;
  }
}

bool RocksDBSessionStore::SetSession(const SessionData &session) {
  if (!impl_ || !impl_->db)
    return false;
  std::string key = std::string(kSessionPrefix) + session.user_id;
  std::string value = SerializeSession(session);
  auto status = impl_->db->Put(rocksdb::WriteOptions(), key, value);
  return status.ok();
}

std::optional<SessionData>
RocksDBSessionStore::GetSession(const std::string &user_id) {
  if (!impl_ || !impl_->db)
    return std::nullopt;
  std::string key = std::string(kSessionPrefix) + user_id;
  std::string value;
  auto status = impl_->db->Get(rocksdb::ReadOptions(), key, &value);
  if (!status.ok()) {
    return std::nullopt;
  }
  try {
    return DeserializeSession(value);
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

bool RocksDBSessionStore::RemoveSession(const std::string &user_id) {
  if (!impl_ || !impl_->db)
    return false;
  std::string key = std::string(kSessionPrefix) + user_id;
  auto status = impl_->db->Delete(rocksdb::WriteOptions(), key);
  return status.ok();
}

} // namespace swift::online
