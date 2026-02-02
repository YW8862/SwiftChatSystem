/**
 * @file group_rpc_client.h
 * @brief GroupService gRPC 客户端
 */

#pragma once

#include "rpc_client_base.h"
// #include "group.grpc.pb.h"

namespace swift {
namespace zone {

/**
 * @class GroupRpcClient
 * @brief GroupService 的 gRPC 客户端封装
 */
class GroupRpcClient : public RpcClientBase {
public:
    GroupRpcClient() = default;
    ~GroupRpcClient() override = default;

    void InitStub();

    // ============ RPC 方法 ============

    /// 创建群组
    // swift::group::CreateGroupResponse CreateGroup(const swift::group::CreateGroupRequest& request);

    /// 解散群组
    // swift::common::CommonResponse DismissGroup(const std::string& group_id, const std::string& operator_id);

    /// 获取群成员列表
    // swift::group::GroupMembersResponse GetGroupMembers(const std::string& group_id);

    /// 获取群信息
    // swift::group::GroupInfoResponse GetGroupInfo(const std::string& group_id);

private:
    // std::unique_ptr<swift::group::GroupService::Stub> stub_;
};

}  // namespace zone
}  // namespace swift
