#pragma once

#include <string>
#include <memory>
#include <optional>
#include <cstdint>
#include "../store/session_store.h"

namespace swift::online {

class OnlineService {
public:
    explicit OnlineService(std::shared_ptr<SessionStore> store,
                           const std::string& jwt_secret = "swift_online_secret_2026");
    ~OnlineService();

    struct LoginResult {
        bool success = false;
        std::string token;
        int64_t expire_at = 0;
        std::string error;
    };
    LoginResult Login(const std::string& user_id,
                      const std::string& device_id = "",
                      const std::string& device_type = "");

    struct LogoutResult {
        bool success = true;
        std::string error;
    };
    LogoutResult Logout(const std::string& user_id, const std::string& token);

    struct TokenResult {
        bool valid = false;
        std::string user_id;
    };
    TokenResult ValidateToken(const std::string& token);

private:
    std::pair<std::string, int64_t> GenerateToken(const std::string& user_id);
    std::string NormalizeDeviceId(const std::string& device_id) const;

    std::shared_ptr<SessionStore> store_;
    std::string jwt_secret_;
};

}  // namespace swift::online
