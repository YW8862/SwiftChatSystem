#pragma once

#include <memory>

namespace swift::online {

class OnlineService;

/**
 * 对外 API 层（Handler）
 * 实现 proto 定义的 gRPC OnlineService 接口：Login、Logout、ValidateToken。
 */
class OnlineHandler {
public:
    explicit OnlineHandler(std::shared_ptr<OnlineService> service);
    ~OnlineHandler();

    // gRPC 接口实现
    // Status Login(ServerContext*, const LoginRequest*, LoginResponse*);
    // Status Logout(ServerContext*, const LogoutRequest*, CommonResponse*);
    // Status ValidateToken(ServerContext*, const TokenRequest*, TokenResponse*);

private:
    std::shared_ptr<OnlineService> service_;
};

}  // namespace swift::online
