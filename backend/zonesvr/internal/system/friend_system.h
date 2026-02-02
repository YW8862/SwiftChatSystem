/**
 * @file friend_system.h
 * @brief 好友子系统
 * 
 * FriendSystem 负责处理好友关系相关的请求：
 * - 添加/删除好友
 * - 好友请求处理
 * - 好友分组管理
 * - 黑名单管理
 * - 好友在线状态
 * 
 * 内部调用 FriendSvr 的 gRPC 接口完成实际业务。
 */

#pragma once

#include "base_system.h"
#include <memory>
#include <string>
#include <vector>

namespace swift {
namespace zone {

// 前向声明
class FriendRpcClient;

/**
 * @class FriendSystem
 * @brief 好友子系统
 */
class FriendSystem : public BaseSystem {
public:
    FriendSystem();
    ~FriendSystem() override;

    std::string Name() const override { return "FriendSystem"; }
    bool Init() override;
    void Shutdown() override;

    // ============ 业务接口 ============

    /// 发送好友请求
    bool AddFriend(const std::string& user_id, 
                   const std::string& friend_id, 
                   const std::string& remark);

    /// 处理好友请求
    bool HandleFriendRequest(const std::string& user_id, 
                             const std::string& request_id, 
                             bool accept);

    /// 删除好友
    bool RemoveFriend(const std::string& user_id, const std::string& friend_id);

    /// 获取好友列表（带在线状态）
    // std::vector<FriendInfo> GetFriends(const std::string& user_id);

    /// 拉黑用户
    bool BlockUser(const std::string& user_id, const std::string& target_id);

    /// 取消拉黑
    bool UnblockUser(const std::string& user_id, const std::string& target_id);

    /// 检查是否是好友
    bool IsFriend(const std::string& user_id, const std::string& friend_id);

    /// 检查是否被拉黑
    bool IsBlocked(const std::string& user_id, const std::string& target_id);

private:
    std::unique_ptr<FriendRpcClient> rpc_client_;
};

}  // namespace zone
}  // namespace swift
