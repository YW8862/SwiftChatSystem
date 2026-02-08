/**
 * @file online_service.cpp
 * @brief OnlineSvr 登录会话业务逻辑
 */

#include "online_service.h"
#include <swift/jwt_helper.h>
#include "swift/utils.h"

namespace swift::online {

constexpr int64_t kTokenExpireDays = 7;
constexpr int64_t kMsPerDay = 24 * 3600 * 1000;
constexpr const char* kDefaultDeviceId = "default";

OnlineServiceCore::OnlineServiceCore(std::shared_ptr<SessionStore> store,
                                     const std::string& jwt_secret)
    : store_(std::move(store))
    , jwt_secret_(jwt_secret.empty() ? "swift_online_secret_2026" : jwt_secret) {}

OnlineServiceCore::~OnlineServiceCore() = default;

OnlineServiceCore::LoginResult OnlineServiceCore::Login(
    const std::string& user_id,
    const std::string& device_id,
    const std::string& device_type) {

    (void)device_type;
    LoginResult result;

    if (user_id.empty()) {
        result.error = "user_id required";
        return result;
    }

    if (!store_) {
        result.error = "Session store not available";
        return result;
    }

    std::string normalized_device = NormalizeDeviceId(device_id);
    auto existing_session = store_->GetSession(user_id);
    int64_t now_ms = swift::utils::GetTimestampMs();

    if (existing_session.has_value()) {
        if (existing_session->device_id != normalized_device) {
            result.success = false;
            result.error = "User already logged in on another device";
            return result;
        }
        swift::JwtPayload payload = swift::JwtVerify(existing_session->token, jwt_secret_);
        if (payload.valid && existing_session->expire_at > now_ms) {
            result.success = true;
            result.token = existing_session->token;
            result.expire_at = existing_session->expire_at;
            return result;
        }
    }

    auto [token, expire_at] = GenerateToken(user_id);
    SessionData session{
        .user_id = user_id,
        .device_id = normalized_device,
        .token = token,
        .login_time = now_ms,
        .expire_at = expire_at
    };

    if (!store_->SetSession(session)) {
        result.error = "Failed to persist session";
        return result;
    }

    result.success = true;
    result.token = token;
    result.expire_at = expire_at;
    return result;
}

OnlineServiceCore::LogoutResult OnlineServiceCore::Logout(const std::string& user_id,
                                                   const std::string& token) {
    (void)token;
    LogoutResult result;
    if (!store_) return result;

    auto session = store_->GetSession(user_id);
    if (!session) {
        result.success = true;
        return result;
    }
    store_->RemoveSession(user_id);
    result.success = true;
    return result;
}

OnlineServiceCore::TokenResult OnlineServiceCore::ValidateToken(const std::string& token) {
    TokenResult result;
    if (token.empty()) return result;

    swift::JwtPayload payload = swift::JwtVerify(token, jwt_secret_);
    if (!payload.valid) return result;

    if (!store_) return result;

    auto session = store_->GetSession(payload.user_id);
    if (!session) return result;

    int64_t now_ms = swift::utils::GetTimestampMs();
    if (session->token != token || session->expire_at <= now_ms) {
        store_->RemoveSession(payload.user_id);
        return result;
    }

    result.valid = true;
    result.user_id = payload.user_id;
    return result;
}

std::pair<std::string, int64_t> OnlineServiceCore::GenerateToken(const std::string& user_id) {
    int64_t now_ms = swift::utils::GetTimestampMs();
    int expire_hours = static_cast<int>(kTokenExpireDays * 24);
    std::string token = swift::JwtCreate(user_id, jwt_secret_, expire_hours);
    int64_t expire_at = now_ms + kTokenExpireDays * kMsPerDay;
    return {token, expire_at};
}

std::string OnlineServiceCore::NormalizeDeviceId(const std::string& device_id) const {
    return device_id.empty() ? kDefaultDeviceId : device_id;
}

}  // namespace swift::online
