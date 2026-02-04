/**
 * @file grpc_auth.cpp
 * @brief 从 gRPC metadata 解析 JWT 并校验，返回请求者 user_id
 */

#include "swift/grpc_auth.h"
#include "swift/jwt_helper.h"
#include <grpcpp/server_context.h>
#include <algorithm>
#include <cctype>

namespace swift {

namespace {

const char kAuthHeader[] = "authorization";
const char kTokenHeader[] = "x-token";
const char kBearerPrefix[] = "Bearer ";

std::string GetTokenFromContext(::grpc::ServerContext* context) {
    const auto& meta = context->client_metadata();
    // 优先 authorization: Bearer <token>
    auto it = meta.find(kAuthHeader);
    if (it != meta.end()) {
        std::string v(it->second.data(), it->second.size());
        // 去掉首尾空白
        size_t start = v.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = v.find_last_not_of(" \t\r\n");
        v = v.substr(start, end == std::string::npos ? std::string::npos : end - start + 1);
        if (v.size() >= sizeof(kBearerPrefix) - 1 &&
            v.compare(0, sizeof(kBearerPrefix) - 1, kBearerPrefix) == 0) {
            v = v.substr(sizeof(kBearerPrefix) - 1);
            start = v.find_first_not_of(" \t");
            if (start != std::string::npos) v = v.substr(start);
            return v;
        }
        return v;  // 无 Bearer 前缀也当作整段为 token
    }
    it = meta.find(kTokenHeader);
    if (it != meta.end()) {
        return std::string(it->second.data(), it->second.size());
    }
    return "";
}

}  // namespace

std::string GetAuthenticatedUserId(::grpc::ServerContext* context,
                                   const std::string& jwt_secret) {
    if (!context || jwt_secret.empty()) return "";
    std::string token = GetTokenFromContext(context);
    if (token.empty()) return "";
    JwtPayload payload = JwtVerify(token, jwt_secret);
    if (!payload.valid) return "";
    return payload.user_id;
}

}  // namespace swift
