#pragma once

#include <string>
#include <memory>
#include <optional>
#include <cstdint>
#include <utility>
#include "swift/error_code.h"
#include "../store/user_store.h"

namespace swift::auth {

/**
 * 用户资料（对外可见，不含密码）
 * 注：与 proto UserProfile 对应，业务层用此结构体，Handler 层转成 proto 消息。
 */
struct AuthProfile {
    std::string user_id;
    std::string username;
    std::string nickname;
    std::string avatar_url;
    std::string signature;
    int gender = 0;
    int64_t created_at = 0;
};

/**
 * 认证服务业务逻辑（身份 + 资料；登录/登出/Token 统一走 OnlineSvr）
 * 注：类名带 Core 以区分 proto 生成的 AuthService。
 */
class AuthServiceCore {
public:
    explicit AuthServiceCore(std::shared_ptr<UserStore> store);
    ~AuthServiceCore();

    // 注册
    struct RegisterResult {
        bool success = false;
        std::string user_id;
        std::string error;
        swift::ErrorCode error_code = swift::ErrorCode::OK;
    };
    RegisterResult Register(const std::string& username, const std::string& password,
                            const std::string& nickname, const std::string& email,
                            const std::string& avatar_url = "");

    // 校验用户名密码，返回 user_id 与 profile（ZoneSvr 登录时先调此接口，再调 OnlineSvr.Login）
    struct VerifyCredentialsResult {
        bool success = false;
        std::string user_id;
        std::optional<AuthProfile> profile;
        std::string error;
        swift::ErrorCode error_code = swift::ErrorCode::OK;
    };
    VerifyCredentialsResult VerifyCredentials(const std::string& username, const std::string& password);

    // 获取用户资料
    std::optional<AuthProfile> GetProfile(const std::string& user_id);

    // 更新用户资料（空字符串表示不更新该字段）
    struct UpdateProfileResult {
        bool success = false;
        std::string error;
        swift::ErrorCode error_code = swift::ErrorCode::OK;
    };
    UpdateProfileResult UpdateProfile(const std::string& user_id,
                                      const std::string& nickname,
                                      const std::string& avatar_url,
                                      const std::string& signature);

private:
    std::string GenerateUserId();
    std::string HashPassword(const std::string& password);
    bool VerifyPassword(const std::string& password, const std::string& hash);
    AuthProfile ToProfile(const UserData& user);

    std::shared_ptr<UserStore> store_;
};

}  // namespace swift::auth
