#pragma once

#include <memory>

namespace swift::auth {

class UserStore;
class AuthService;

/**
 * AuthService gRPC 处理器
 * 
 * 实现 proto 中定义的 AuthService 接口
 */
class AuthHandler {
public:
    explicit AuthHandler(std::shared_ptr<AuthService> service);
    ~AuthHandler();
    
    // gRPC 接口实现
    // Status Register(ServerContext*, const RegisterRequest*, RegisterResponse*);
    // Status Login(ServerContext*, const LoginRequest*, LoginResponse*);
    // Status ValidateToken(ServerContext*, const TokenRequest*, TokenResponse*);
    // Status Logout(ServerContext*, const LogoutRequest*, CommonResponse*);
    // Status GetProfile(ServerContext*, const GetProfileRequest*, UserProfile*);
    // Status UpdateProfile(ServerContext*, const UpdateProfileRequest*, CommonResponse*);
    
private:
    std::shared_ptr<AuthService> service_;
};

}  // namespace swift::auth
