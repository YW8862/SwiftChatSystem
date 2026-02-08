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
    bool BlockUser(const std::string& user_id, const std::string& target_id, std::string* out_error,
                   const std::string& token = "");
    bool UnblockUser(const std::string& user_id, const std::string& target_id, std::string* out_error,
                     const std::string& token = "");

private:
    std::unique_ptr<swift::relation::FriendService::Stub> stub_;
};

}  // namespace zone
}  // namespace swift
