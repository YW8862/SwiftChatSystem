/**
 * @file friend_rpc_client.h
 * @brief FriendSvr gRPC 客户端
 */

#pragma once

#include "rpc_client_base.h"
// #include "friend.grpc.pb.h"

namespace swift {
namespace zone {

/**
 * @class FriendRpcClient
 * @brief FriendSvr 的 gRPC 客户端封装
 */
class FriendRpcClient : public RpcClientBase {
public:
    FriendRpcClient() = default;
    ~FriendRpcClient() override = default;

    void InitStub();

    // ============ RPC 方法 ============

    /// 添加好友
    // swift::common::CommonResponse AddFriend(const swift::relation::AddFriendRequest& request);

    /// 处理好友请求
    // swift::common::CommonResponse HandleFriendRequest(const swift::relation::HandleFriendReq& request);

    /// 获取好友列表
    // swift::relation::FriendListResponse GetFriends(const std::string& user_id);

    /// 拉黑用户
    // swift::common::CommonResponse BlockUser(const std::string& user_id, const std::string& target_id);

private:
    // std::unique_ptr<swift::relation::FriendService::Stub> stub_;
};

}  // namespace zone
}  // namespace swift
