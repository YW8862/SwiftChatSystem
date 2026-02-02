/**
 * @file auth_system.h
 * @brief 认证子系统
 * 
 * AuthSystem 负责处理用户认证相关的请求：
 * - 用户登录验证
 * - Token 校验
 * - 用户资料获取/更新
 * 
 * 内部调用 AuthSvr 的 gRPC 接口完成实际业务。
 */

#pragma once

#include "base_system.h"
#include <memory>

namespace swift {
namespace zone {

// 前向声明
class AuthRpcClient;

/**
 * @class AuthSystem
 * @brief 认证子系统
 */
class AuthSystem : public BaseSystem {
public:
    AuthSystem();
    ~AuthSystem() override;

    std::string Name() const override { return "AuthSystem"; }
    bool Init() override;
    void Shutdown() override;

    // ============ 业务接口 ============

    /// 验证 Token 并返回用户ID
    /// @return 用户ID，失败返回空字符串
    std::string ValidateToken(const std::string& token);

    /// 获取用户资料
    // UserProfile GetUserProfile(const std::string& user_id);

    /// 更新用户资料
    // bool UpdateUserProfile(const std::string& user_id, const UserProfile& profile);

private:
    std::unique_ptr<AuthRpcClient> rpc_client_;
};

}  // namespace zone
}  // namespace swift
