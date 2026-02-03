/**
 * @file auth_service.cpp
 * @brief AuthSvr 认证服务业务逻辑
 */

#include "auth_service.h"
#include "swift/utils.h"
#include <algorithm>
#include <cctype>

namespace swift::auth {

// 密码加盐
constexpr const char *kPasswordSalt = "swift_salt_2026";

// ============================================================================
// 辅助函数
// ============================================================================

namespace {

/// 校验用户名：字母数字下划线，3-32 位
bool ValidateUsername(const std::string &username) {
  if (username.size() < 3 || username.size() > 32)
    return false;
  for (char c : username) {
    if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_')
      return false;
  }
  return true;
}

/// 校验密码：至少 6 位
bool ValidatePassword(const std::string &password) {
  return password.size() >= 8;
}

} // namespace

// ============================================================================
// 构造/析构
// ============================================================================

AuthServiceCore::AuthServiceCore(std::shared_ptr<UserStore> store)
    : store_(std::move(store)) {}

AuthServiceCore::~AuthServiceCore() = default;

// ============================================================================
// Register
// ============================================================================

AuthServiceCore::RegisterResult
AuthServiceCore::Register(const std::string &username,
                          const std::string &password,
                          const std::string &nickname, const std::string &email,
                          const std::string &avatar_url) {

  RegisterResult result;

  if (!ValidateUsername(username)) {
    result.success = false;
    result.error = "Username must be 3-32 chars, alphanumeric and underscore";
    result.error_code = swift::ErrorCode::USERNAME_INVALID;
    return result;
  }

  if (!ValidatePassword(password)) {
    result.success = false;
    result.error = "Password must be at least 6 characters";
    result.error_code = swift::ErrorCode::PASSWORD_TOO_WEAK;
    return result;
  }

  if (store_->UsernameExists(username)) {
    result.success = false;
    result.error = "Username already exists";
    result.error_code = swift::ErrorCode::USER_ALREADY_EXISTS;
    return result;
  }

  std::string user_id = GenerateUserId();
  std::string password_hash = HashPassword(password);
  int64_t now = swift::utils::GetTimestampMs();

  UserData user;
  user.user_id = user_id;
  user.username = username;
  user.password_hash = password_hash;
  user.nickname = nickname.empty() ? username : nickname;
  user.avatar_url = avatar_url;
  user.signature = "";
  user.gender = 0;
  user.created_at = now;
  user.updated_at = now;

  if (store_->Create(user)) {
    result.success = true;
    result.user_id = user_id;
  } else {
    result.success = false;
    result.error = "Failed to create user";
    result.error_code = swift::ErrorCode::ROCKSDB_ERROR;
  }

  return result;
}

// ============================================================================
// VerifyCredentials
// ============================================================================

AuthServiceCore::VerifyCredentialsResult
AuthServiceCore::VerifyCredentials(const std::string &username,
                                   const std::string &password) {

  VerifyCredentialsResult result;

  auto user = store_->GetByUsername(username);
  if (!user) {
    result.success = false;
    result.error = "User not found";
    result.error_code = swift::ErrorCode::USER_NOT_FOUND;
    return result;
  }

  if (!VerifyPassword(password, user->password_hash)) {
    result.success = false;
    result.error = "Wrong password";
    result.error_code = swift::ErrorCode::PASSWORD_WRONG;
    return result;
  }

  result.success = true;
  result.user_id = user->user_id;
  result.profile = ToProfile(*user);
  return result;
}

// ============================================================================
// GetProfile
// ============================================================================

std::optional<AuthProfile>
AuthServiceCore::GetProfile(const std::string &user_id) {
  auto user = store_->GetById(user_id);
  if (!user)
    return std::nullopt;
  return ToProfile(*user);
}

// ============================================================================
// UpdateProfile
// ============================================================================

AuthServiceCore::UpdateProfileResult AuthServiceCore::UpdateProfile(
    const std::string &user_id, const std::string &nickname,
    const std::string &avatar_url, const std::string &signature) {

  UpdateProfileResult result;

  auto user = store_->GetById(user_id);
  if (!user) {
    result.success = false;
    result.error = "User not found";
    result.error_code = swift::ErrorCode::USER_NOT_FOUND;
    return result;
  }

  if (!nickname.empty())
    user->nickname = nickname;
  if (!avatar_url.empty())
    user->avatar_url = avatar_url;
  if (!signature.empty())
    user->signature = signature;

  user->updated_at = swift::utils::GetTimestampMs();

  if (store_->Update(*user)) {
    result.success = true;
  } else {
    result.success = false;
    result.error = "Failed to update profile";
    result.error_code = swift::ErrorCode::ROCKSDB_ERROR;
  }

  return result;
}

// ============================================================================
// 私有方法
// ============================================================================

std::string AuthServiceCore::GenerateUserId() {
  return swift::utils::GenerateShortId("u_", 12);
}

std::string AuthServiceCore::HashPassword(const std::string &password) {
  return swift::utils::SHA256(password + kPasswordSalt);
}

bool AuthServiceCore::VerifyPassword(const std::string &password,
                                     const std::string &hash) {
  return HashPassword(password) == hash;
}

AuthProfile AuthServiceCore::ToProfile(const UserData &user) {
  AuthProfile p;
  p.user_id = user.user_id;
  p.username = user.username;
  p.nickname = user.nickname;
  p.avatar_url = user.avatar_url;
  p.signature = user.signature;
  p.gender = user.gender;
  p.created_at = user.created_at;
  return p;
}

} // namespace swift::auth
