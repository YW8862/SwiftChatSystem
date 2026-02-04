#pragma once

/**
 * @file grpc_auth.h
 * @brief 从 gRPC 请求中解析并校验 JWT，得到当前请求者 user_id
 *
 * 约定：客户端在 gRPC metadata 中携带 Token，任选其一：
 *   - authorization: "Bearer <jwt>"
 *   - x-token: "<jwt>"
 *
 * 业务服务（FriendSvr、ChatSvr 等）在 Handler 中调用 GetAuthenticatedUserId，
 * 用返回的 user_id 作为「当前用户」，不再信任请求体中的 user_id，从而防止伪造越权。
 */

#include <grpcpp/server_context.h>
#include <string>

namespace swift {

/**
 * 从 ServerContext 的 metadata 中读取 JWT 并校验，返回当前用户 ID。
 * @param context gRPC ServerContext（从中读 client_metadata）
 * @param jwt_secret 与 OnlineSvr 签发 Token 时相同的密钥
 * @return 校验成功返回 user_id，否则返回空字符串（未带 token / token 无效 / 过期）
 */
std::string GetAuthenticatedUserId(::grpc::ServerContext* context,
                                   const std::string& jwt_secret);

}  // namespace swift
