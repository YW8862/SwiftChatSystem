#pragma once

#include <memory>
#include "group.grpc.pb.h"

namespace swift::group_ {

class GroupService;  // 业务逻辑，与 proto 的 swift::group::GroupService 区分

/**
 * 群组 gRPC API 层
 * 实现 proto 定义的 GroupService：创建群（至少 3 人）、邀请成员等。
 */
class GroupHandler : public ::swift::group::GroupService::Service {
public:
    explicit GroupHandler(std::shared_ptr<GroupService> service);
    ~GroupHandler() override;

    ::grpc::Status CreateGroup(::grpc::ServerContext* context,
                               const ::swift::group::CreateGroupRequest* request,
                               ::swift::group::CreateGroupResponse* response) override;

    ::grpc::Status DismissGroup(::grpc::ServerContext* context,
                               const ::swift::group::DismissGroupRequest* request,
                               ::swift::common::CommonResponse* response) override;

    ::grpc::Status GetGroupInfo(::grpc::ServerContext* context,
                                const ::swift::group::GetGroupInfoRequest* request,
                                ::swift::group::GroupInfoResponse* response) override;

    ::grpc::Status UpdateGroup(::grpc::ServerContext* context,
                               const ::swift::group::UpdateGroupRequest* request,
                               ::swift::common::CommonResponse* response) override;

    ::grpc::Status InviteMembers(::grpc::ServerContext* context,
                                  const ::swift::group::InviteMembersRequest* request,
                                  ::swift::common::CommonResponse* response) override;

    ::grpc::Status RemoveMember(::grpc::ServerContext* context,
                                 const ::swift::group::RemoveMemberRequest* request,
                                 ::swift::common::CommonResponse* response) override;

    ::grpc::Status LeaveGroup(::grpc::ServerContext* context,
                              const ::swift::group::LeaveGroupRequest* request,
                              ::swift::common::CommonResponse* response) override;

    ::grpc::Status GetGroupMembers(::grpc::ServerContext* context,
                                   const ::swift::group::GetGroupMembersRequest* request,
                                   ::swift::group::GroupMembersResponse* response) override;

    ::grpc::Status TransferOwner(::grpc::ServerContext* context,
                                  const ::swift::group::TransferOwnerRequest* request,
                                  ::swift::common::CommonResponse* response) override;

    ::grpc::Status SetMemberRole(::grpc::ServerContext* context,
                                 const ::swift::group::SetMemberRoleRequest* request,
                                 ::swift::common::CommonResponse* response) override;

    ::grpc::Status GetUserGroups(::grpc::ServerContext* context,
                                 const ::swift::group::GetUserGroupsRequest* request,
                                 ::swift::group::UserGroupsResponse* response) override;

    ::grpc::Status MuteGroup(::grpc::ServerContext* context,
                              const ::swift::group::MuteGroupRequest* request,
                              ::swift::common::CommonResponse* response) override;

private:
    std::shared_ptr<GroupService> service_;
};

}  // namespace swift::group_
