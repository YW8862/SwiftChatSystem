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
#include "../rpc/friend_rpc_client.h"
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

    /// 发送好友请求；token 用于调 FriendSvr 时注入 metadata
    bool AddFriend(const std::string& user_id, const std::string& friend_id,
                   const std::string& remark, const std::string& token = "");

    /// 处理好友请求
    bool HandleFriendRequest(const std::string& user_id, const std::string& request_id,
                             bool accept, const std::string& token = "");

    /// 删除好友
    bool RemoveFriend(const std::string& user_id, const std::string& friend_id,
                     const std::string& token = "");

    /// 获取好友列表（group_id 为空表示全部）
    bool GetFriends(const std::string& user_id, const std::string& group_id,
                    std::vector<FriendInfoResult>* out_friends, std::string* out_error,
                    const std::string& token = "");

    /// 获取好友申请列表（type: 0=全部, 1=收到的, 2=发出的）
    bool GetFriendRequests(const std::string& user_id, int32_t type,
                           std::vector<FriendRequestInfoResult>* out_requests, std::string* out_error,
                           const std::string& token = "");

    /// 拉黑用户
    bool BlockUser(const std::string& user_id, const std::string& target_id,
                   const std::string& token = "");

    /// 取消拉黑
    bool UnblockUser(const std::string& user_id, const std::string& target_id,
                     const std::string& token = "");

    /// 检查是否是好友
    bool IsFriend(const std::string& user_id, const std::string& friend_id);

    /// 检查是否被拉黑
    bool IsBlocked(const std::string& user_id, const std::string& target_id);

private:
    std::unique_ptr<FriendRpcClient> rpc_client_;
};

}  // namespace zone
}  // namespace swift
