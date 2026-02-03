#pragma once

#include <memory>
#include <optional>
#include <string>

namespace swift::auth {

/**
 * 用户存储数据结构
 */
struct UserData {
  std::string user_id;
  std::string username;
  std::string password_hash;
  std::string nickname;
  std::string avatar_url;
  std::string signature;
  int gender = 0;
  int64_t created_at = 0;
  int64_t updated_at = 0;
};

/**
 * 用户存储接口
 *
 * RocksDB Key 设计：
 *   user:{user_id}          -> UserData (JSON)
 *   username:{username}     -> user_id (用于登录查询)
 */
class UserStore {
public:
  virtual ~UserStore() = default;

  // 创建用户
  virtual bool Create(const UserData &user) = 0;

  // 根据 user_id 查询
  virtual std::optional<UserData> GetById(const std::string &user_id) = 0;

  // 根据 username 查询
  virtual std::optional<UserData>
  GetByUsername(const std::string &username) = 0;

  // 更新用户信息
  virtual bool Update(const UserData &user) = 0;

  // 检查用户名是否存在
  virtual bool UsernameExists(const std::string &username) = 0;
};

/**
 * RocksDB 实现
 */
class RocksDBUserStore : public UserStore {
public:
  explicit RocksDBUserStore(const std::string &db_path);
  ~RocksDBUserStore() override;

  bool Create(const UserData &user) override;
  std::optional<UserData> GetById(const std::string &user_id) override;
  std::optional<UserData> GetByUsername(const std::string &username) override;
  bool Update(const UserData &user) override;
  bool UsernameExists(const std::string &username) override;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace swift::auth
