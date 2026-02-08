/**
 * @file group_rpc_client.h
 * @brief GroupService gRPC 客户端
 */

#pragma once

#include "rpc_client_base.h"
#include "group.grpc.pb.h"
#include <memory>
#include <string>
#include <vector>
#include <cstdint>

namespace swift {
namespace zone {

struct GroupInfoResult {
    std::string group_id;
    std::string group_name;
    std::string avatar_url;
    std::string owner_id;
    int32_t member_count = 0;
    std::string announcement;
    int64_t created_at = 0;
};

struct GroupMemberResult {
    std::string user_id;
    int32_t role = 0;
    std::string nickname;
    int64_t joined_at = 0;
};

struct CreateGroupResult {
    bool success = false;
    std::string group_id;
    std::string error;
};

/**
 * @class GroupRpcClient
 * @brief GroupService 的 gRPC 客户端封装
 */
class GroupRpcClient : public RpcClientBase {
public:
    GroupRpcClient() = default;
    ~GroupRpcClient() override = default;

    void InitStub();

    CreateGroupResult CreateGroup(const std::string& creator_id, const std::string& group_name,
                                  const std::string& avatar_url,
                                  const std::vector<std::string>& member_ids);
    bool DismissGroup(const std::string& group_id, const std::string& operator_id, std::string* out_error);
    bool GetGroupInfo(const std::string& group_id, GroupInfoResult* out_info, std::string* out_error);
    bool GetGroupMembers(const std::string& group_id, int32_t page, int32_t page_size,
                         std::vector<GroupMemberResult>* out_members, int32_t* total, std::string* out_error);
    bool InviteMembers(const std::string& group_id, const std::string& inviter_id,
                       const std::vector<std::string>& member_ids, std::string* out_error);
    bool RemoveMember(const std::string& group_id, const std::string& operator_id,
                      const std::string& member_id, std::string* out_error);
    bool LeaveGroup(const std::string& group_id, const std::string& user_id, std::string* out_error);
    bool TransferOwner(const std::string& group_id, const std::string& old_owner_id,
                       const std::string& new_owner_id, std::string* out_error);

private:
    std::unique_ptr<swift::group::GroupService::Stub> stub_;
};

}  // namespace zone
}  // namespace swift
