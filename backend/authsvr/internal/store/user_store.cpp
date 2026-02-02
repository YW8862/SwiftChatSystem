#include "user_store.h"
// #include <rocksdb/db.h>
// #include <nlohmann/json.hpp>

namespace swift::auth {

struct RocksDBUserStore::Impl {
    // rocksdb::DB* db = nullptr;
    std::string db_path;
};

RocksDBUserStore::RocksDBUserStore(const std::string& db_path) 
    : impl_(std::make_unique<Impl>()) {
    impl_->db_path = db_path;
    
    // TODO: 打开 RocksDB
    // rocksdb::Options options;
    // options.create_if_missing = true;
    // rocksdb::DB::Open(options, db_path, &impl_->db);
}

RocksDBUserStore::~RocksDBUserStore() {
    // delete impl_->db;
}

bool RocksDBUserStore::Create(const UserData& user) {
    // TODO: 实现
    // 1. 检查 username 是否已存在
    // 2. 写入 user:{user_id} -> UserData JSON
    // 3. 写入 username:{username} -> user_id
    return false;
}

std::optional<UserData> RocksDBUserStore::GetById(const std::string& user_id) {
    // TODO: 读取 user:{user_id}
    return std::nullopt;
}

std::optional<UserData> RocksDBUserStore::GetByUsername(const std::string& username) {
    // TODO: 
    // 1. 读取 username:{username} 获取 user_id
    // 2. 读取 user:{user_id}
    return std::nullopt;
}

bool RocksDBUserStore::Update(const UserData& user) {
    // TODO: 更新 user:{user_id}
    return false;
}

bool RocksDBUserStore::UsernameExists(const std::string& username) {
    // TODO: 检查 username:{username} 是否存在
    return false;
}

}  // namespace swift::auth
