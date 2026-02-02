#pragma once

#include <string>
#include <memory>
#include <optional>
#include "../store/user_store.h"

namespace swift::auth {

/**
 * 认证服务业务逻辑
 */
class AuthService {
public:
    explicit AuthService(std::shared_ptr<UserStore> store);
    ~AuthService();
    
    // 注册
    struct RegisterResult {
        bool success;
        std::string user_id;
        std::string error;
    };
    RegisterResult Register(const std::string& username, const std::string& password,
                            const std::string& nickname, const std::string& email);
    
    // 登录
    struct LoginResult {
        bool success;
        std::string user_id;
        std::string token;
        int64_t expire_at;
        std::string error;
    };
    LoginResult Login(const std::string& username, const std::string& password,
                      const std::string& device_id, const std::string& device_type);
    
    // 验证 Token
    struct TokenResult {
        bool valid;
        std::string user_id;
    };
    TokenResult ValidateToken(const std::string& token);
    
    // 获取用户信息
    std::optional<UserData> GetProfile(const std::string& user_id);
    
    // 更新用户信息
    bool UpdateProfile(const std::string& user_id, const std::string& nickname,
                       const std::string& avatar_url, const std::string& signature);
    
private:
    std::string GenerateUserId();
    std::string HashPassword(const std::string& password);
    bool VerifyPassword(const std::string& password, const std::string& hash);
    std::string GenerateToken(const std::string& user_id);
    
    std::shared_ptr<UserStore> store_;
};

}  // namespace swift::auth
