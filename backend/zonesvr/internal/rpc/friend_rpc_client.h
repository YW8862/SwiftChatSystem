/**
 * @file friend_rpc_client.h
 * @brief FriendSvr gRPC 客户端
 */

#pragma once

#include "rpc_client_base.h"
#include "friend.grpc.pb.h"
#include <memory>
#include <string>
#include <vector>
#include <cstdint>

namespace swift {
namespace zone {

struct FriendInfoResult {
    std::string friend_id;
    std::string remark;
    std::string group_id;
    std::string nickname;
    std::string avatar_url;
    int64_t added_at = 0;
};

struct FriendRequestInfoResult {
    std::string request_id;
    std::string from_user_id;
    std::string to_user_id;
    std::string remark;
    int32_t status = 0;
    int64_t created_at = 0;
    std::string from_nickname;
    std::string from_avatar_url;
};

struct FriendGroupResult {
    std::string group_id;
    std::string group_name;
    int32_t friend_count = 0;
    int32_t sort_order = 0;
};

/**
 * @class FriendRpcClient
 * @brief FriendSvr 的 gRPC 客户端封装
 */
class FriendRpcClient : public RpcClientBase {
public:
    FriendRpcClient() = default;
    ~FriendRpcClient() override = default;

    void InitStub();

    bool AddFriend(const std::string& user_id, const std::string& friend_id,
                   const std::string& remark, const std::string& group_id, std::string* out_error,
                   const std::string& token = "");
    bool HandleFriendRequest(const std::string& user_id, const std::string& request_id,
                             bool accept, const std::string& group_id, std::string* out_error,
                             const std::string& token = "");
    bool RemoveFriend(const std::string& user_id, const std::string& friend_id, std::string* out_error,
                      const std::string& token = "");
    bool GetFriends(const std::string& user_id, const std::string& group_id,
                    std::vector<FriendInfoResult>* out_friends, std::string* out_error,
                    const std::string& token = "");
    bool GetFriendRequests(const std::string& user_id, int32_t type,
                          std::vector<FriendRequestInfoResult>* out_requests, std::string* out_error,
                          const std::string& token = "");
    bool BlockUser(const std::string& user_id, const std::string& target_id, std::string* out_error,
                   const std::string& token = "");
    bool UnblockUser(const std::string& user_id, const std::string& target_id, std::string* out_error,
                     const std::string& token = "");
    /// 获取黑名单 ID 列表，用于 IsBlocked 等
    bool GetBlockList(const std::string& user_id, std::vector<std::string>* out_blocked_ids,
                      std::string* out_error, const std::string& token = "");
    /// 创建好友分组
    bool CreateFriendGroup(const std::string& user_id, const std::string& group_name,
                           std::string* out_group_id, std::string* out_error,
                           const std::string& token = "");
    /// 获取好友分组列表
    bool GetFriendGroups(const std::string& user_id, std::vector<FriendGroupResult>* out_groups,
                         std::string* out_error, const std::string& token = "");
    /// 移动好友到分组
    bool MoveFriendToGroup(const std::string& user_id, const std::string& friend_id,
                           const std::string& group_id, std::string* out_error,
                           const std::string& token = "");
    /// 删除好友分组
    bool DeleteFriendGroup(const std::string& user_id, const std::string& group_id,
                           std::string* out_error, const std::string& token = "");
    /// 设置好友备注
    bool SetRemark(const std::string& user_id, const std::string& friend_id,
                   const std::string& remark, std::string* out_error,
                   const std::string& token = "");

private:
    std::unique_ptr<swift::relation::FriendService::Stub> stub_;
};

}  // namespace zone
}  // namespace swift
