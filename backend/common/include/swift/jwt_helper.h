#pragma once

/**
 * @file jwt_helper.h
 * @brief 公共 JWT 工具（HS256）
 *
 * 使用 OpenSSL HMAC-SHA256 + nlohmann/json，供 OnlineSvr 等签发/校验 Token。
 * 放在 common 避免 authsvr/onlinesvr 重复实现。
 */

#include <string>
#include <cstdint>

namespace swift {

struct JwtPayload {
    std::string user_id;   // sub
    std::string issuer;    // iss
    int64_t iat = 0;
    int64_t exp = 0;
    bool valid = false;
};

/**
 * @brief 生成 JWT Token
 * @param user_id 用户 ID（sub）
 * @param secret 密钥
 * @param expire_hours 过期小时数
 * @param issuer 签发者标识，默认 "swift-online"
 */
std::string JwtCreate(const std::string& user_id,
                      const std::string& secret,
                      int expire_hours = 24 * 7,
                      const std::string& issuer = "swift-online");

/**
 * @brief 验证并解码 JWT Token
 * @param token JWT 字符串
 * @param secret 密钥
 * @return 解码结果，验证失败 valid=false；接受 iss 为 swift-online / swift-auth
 */
JwtPayload JwtVerify(const std::string& token, const std::string& secret);

}  // namespace swift
