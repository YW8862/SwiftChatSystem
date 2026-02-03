/**
 * @file group_handler.cpp
 * @brief 群组 gRPC API：将请求转调 GroupService，结果写入 proto 响应。
 */

#include "group_handler.h"
#include "../service/group_service.h"
#include "swift/error_code.h"
#include <grpcpp/server_context.h>

namespace swift::group_ {

GroupHandler::GroupHandler(std::shared_ptr<GroupService> service)
    : service_(std::move(service)) {}

GroupHandler::~GroupHandler() = default;

namespace {

void SetCommonOk(::swift::common::CommonResponse* response) {
    response->set_code(static_cast<int>(swift::ErrorCode::OK));
}

void SetCommonFail(::swift::common::CommonResponse* response,
                   swift::ErrorCode code,
                   const std::string& message = "") {
    response->set_code(swift::ErrorCodeToInt(code));
    response->set_message(message.empty() ? swift::ErrorCodeToString(code) : message);
}

void FillGroupInfo(::swift::group::GroupInfo* out, const GroupData& g) {
    out->set_group_id(g.group_id);
    out->set_group_name(g.group_name);
    out->set_avatar_url(g.avatar_url);
    out->set_owner_id(g.owner_id);
    out->set_member_count(g.member_count);
    out->set_announcement(g.announcement);
    out->set_created_at(g.created_at);
    out->set_updated_at(g.updated_at);
}

void FillGroupMember(::swift::group::GroupMember* out, const GroupMemberData& m) {
    out->set_user_id(m.user_id);
    out->set_role(m.role);
    out->set_nickname(m.nickname);
    out->set_joined_at(m.joined_at);
    // profile 由客户端或其它服务补充
}

}  // namespace

::grpc::Status GroupHandler::CreateGroup(::grpc::ServerContext* context,
                                         const ::swift::group::CreateGroupRequest* request,
                                         ::swift::group::CreateGroupResponse* response) {
    (void)context;
    std::vector<std::string> member_ids(request->member_ids().begin(), request->member_ids().end());
    auto result = service_->CreateGroup(request->creator_id(), request->group_name(),
                                        request->avatar_url(), member_ids);
    response->set_code(swift::ErrorCodeToInt(result.error_code));
    response->set_message(swift::ErrorCodeToString(result.error_code));
    if (result.error_code == swift::ErrorCode::OK)
        response->set_group_id(result.group_id);
    return ::grpc::Status::OK;
}

::grpc::Status GroupHandler::DismissGroup(::grpc::ServerContext* context,
                                           const ::swift::group::DismissGroupRequest* request,
                                           ::swift::common::CommonResponse* response) {
    (void)context;
    swift::ErrorCode code = service_->DismissGroup(request->group_id(), request->operator_id());
    if (code == swift::ErrorCode::OK)
        SetCommonOk(response);
    else
        SetCommonFail(response, code);
    return ::grpc::Status::OK;
}

::grpc::Status GroupHandler::GetGroupInfo(::grpc::ServerContext* context,
                                           const ::swift::group::GetGroupInfoRequest* request,
                                           ::swift::group::GroupInfoResponse* response) {
    (void)context;
    auto g = service_->GetGroupInfo(request->group_id());
    if (!g) {
        response->set_code(swift::ErrorCodeToInt(swift::ErrorCode::GROUP_NOT_FOUND));
        response->set_message(swift::ErrorCodeToString(swift::ErrorCode::GROUP_NOT_FOUND));
        return ::grpc::Status::OK;
    }
    response->set_code(static_cast<int>(swift::ErrorCode::OK));
    FillGroupInfo(response->mutable_group(), *g);
    return ::grpc::Status::OK;
}

::grpc::Status GroupHandler::UpdateGroup(::grpc::ServerContext* context,
                                           const ::swift::group::UpdateGroupRequest* request,
                                           ::swift::common::CommonResponse* response) {
    (void)context;
    swift::ErrorCode code = service_->UpdateGroup(request->group_id(), request->operator_id(),
                                                   request->group_name(), request->avatar_url(),
                                                   request->announcement());
    if (code == swift::ErrorCode::OK)
        SetCommonOk(response);
    else
        SetCommonFail(response, code);
    return ::grpc::Status::OK;
}

::grpc::Status GroupHandler::InviteMembers(::grpc::ServerContext* context,
                                            const ::swift::group::InviteMembersRequest* request,
                                            ::swift::common::CommonResponse* response) {
    (void)context;
    std::vector<std::string> member_ids(request->member_ids().begin(), request->member_ids().end());
    swift::ErrorCode code = service_->InviteMembers(request->group_id(), request->inviter_id(),
                                                     member_ids);
    if (code == swift::ErrorCode::OK)
        SetCommonOk(response);
    else
        SetCommonFail(response, code);
    return ::grpc::Status::OK;
}

::grpc::Status GroupHandler::RemoveMember(::grpc::ServerContext* context,
                                            const ::swift::group::RemoveMemberRequest* request,
                                            ::swift::common::CommonResponse* response) {
    (void)context;
    swift::ErrorCode code = service_->RemoveMember(request->group_id(), request->operator_id(),
                                                    request->member_id());
    if (code == swift::ErrorCode::OK)
        SetCommonOk(response);
    else
        SetCommonFail(response, code);
    return ::grpc::Status::OK;
}

::grpc::Status GroupHandler::LeaveGroup(::grpc::ServerContext* context,
                                         const ::swift::group::LeaveGroupRequest* request,
                                         ::swift::common::CommonResponse* response) {
    (void)context;
    swift::ErrorCode code = service_->LeaveGroup(request->group_id(), request->user_id());
    if (code == swift::ErrorCode::OK)
        SetCommonOk(response);
    else
        SetCommonFail(response, code);
    return ::grpc::Status::OK;
}

::grpc::Status GroupHandler::GetGroupMembers(::grpc::ServerContext* context,
                                              const ::swift::group::GetGroupMembersRequest* request,
                                              ::swift::group::GroupMembersResponse* response) {
    (void)context;
    int page = request->page() > 0 ? request->page() : 1;
    int page_size = request->page_size() > 0 ? request->page_size() : 50;
    int total = 0;
    auto members = service_->GetGroupMembers(request->group_id(), page, page_size, &total);
    response->set_code(static_cast<int>(swift::ErrorCode::OK));
    response->set_total(total);
    for (const auto& m : members) {
        FillGroupMember(response->add_members(), m);
    }
    return ::grpc::Status::OK;
}

::grpc::Status GroupHandler::TransferOwner(::grpc::ServerContext* context,
                                             const ::swift::group::TransferOwnerRequest* request,
                                             ::swift::common::CommonResponse* response) {
    (void)context;
    swift::ErrorCode code = service_->TransferOwner(request->group_id(), request->old_owner_id(),
                                                     request->new_owner_id());
    if (code == swift::ErrorCode::OK)
        SetCommonOk(response);
    else
        SetCommonFail(response, code);
    return ::grpc::Status::OK;
}

::grpc::Status GroupHandler::SetMemberRole(::grpc::ServerContext* context,
                                            const ::swift::group::SetMemberRoleRequest* request,
                                            ::swift::common::CommonResponse* response) {
    (void)context;
    swift::ErrorCode code = service_->SetMemberRole(request->group_id(), request->operator_id(),
                                                     request->member_id(), request->role());
    if (code == swift::ErrorCode::OK)
        SetCommonOk(response);
    else
        SetCommonFail(response, code);
    return ::grpc::Status::OK;
}

::grpc::Status GroupHandler::GetUserGroups(::grpc::ServerContext* context,
                                            const ::swift::group::GetUserGroupsRequest* request,
                                            ::swift::group::UserGroupsResponse* response) {
    (void)context;
    auto groups = service_->GetUserGroups(request->user_id());
    response->set_code(static_cast<int>(swift::ErrorCode::OK));
    for (const auto& g : groups) {
        FillGroupInfo(response->add_groups(), g);
    }
    return ::grpc::Status::OK;
}

::grpc::Status GroupHandler::MuteGroup(::grpc::ServerContext* context,
                                         const ::swift::group::MuteGroupRequest* request,
                                         ::swift::common::CommonResponse* response) {
    (void)context;
    (void)request;
    // 免打扰为客户端本地设置，服务端暂不持久化
    SetCommonOk(response);
    return ::grpc::Status::OK;
}

}  // namespace swift::group_
