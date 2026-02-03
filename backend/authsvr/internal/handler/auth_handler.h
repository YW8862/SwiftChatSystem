#pragma once

#include <memory>

namespace swift::auth {

class UserStore;
class AuthService;

/**
 * 对外 API 层（Handler）
 * 
 * 直接实现 proto 定义的 gRPC AuthService 接口，无独立 API 层。
 * 解析请求、调用 AuthService、组装响应。
 */
class AuthHandler {
public:
    explicit AuthHandler(std::shared_ptr<AuthService> service);
    ~AuthHandler();
    
    // gRPC 接口实现
    // Status Register(ServerContext*, const RegisterRequest*, RegisterResponse*);
    // Status VerifyCredentials(ServerContext*, const VerifyCredentialsRequest*, VerifyCredentialsResponse*);
    // Status GetProfile(ServerContext*, const GetProfileRequest*, UserProfile*);
    // Status UpdateProfile(ServerContext*, const UpdateProfileRequest*, CommonResponse*);
    
private:
    std::shared_ptr<AuthService> service_;
};

}  // namespace swift::auth
