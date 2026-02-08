/**
 * @file group_rpc_client.cpp
 * @brief GroupRpcClient 实现
 */

#include "group_rpc_client.h"

namespace swift {
namespace zone {

void GroupRpcClient::InitStub() {
    stub_ = swift::group::GroupService::NewStub(GetChannel());
}

CreateGroupResult GroupRpcClient::CreateGroup(const std::string& creator_id,
                                              const std::string& group_name,
                                              const std::string& avatar_url,
                                              const std::vector<std::string>& member_ids) {
    CreateGroupResult result;
    if (!stub_) return result;
    swift::group::CreateGroupRequest req;
    req.set_creator_id(creator_id);
    req.set_group_name(group_name);
    if (!avatar_url.empty()) req.set_avatar_url(avatar_url);
    for (const auto& id : member_ids) req.add_member_ids(id);
    swift::group::CreateGroupResponse resp;
    auto ctx = CreateContext(5000);
    grpc::Status status = stub_->CreateGroup(ctx.get(), req, &resp);
    if (!status.ok()) {
        result.error = status.error_message();
        return result;
    }
    if (resp.code() != 0) {
        result.error = resp.message().empty() ? "create group failed" : resp.message();
        return result;
    }
    result.success = true;
    result.group_id = resp.group_id();
    return result;
}

bool GroupRpcClient::DismissGroup(const std::string& group_id, const std::string& operator_id,
                                  std::string* out_error) {
    if (!stub_) return false;
    swift::group::DismissGroupRequest req;
    req.set_group_id(group_id);
    req.set_operator_id(operator_id);
    swift::common::CommonResponse resp;
    auto ctx = CreateContext(5000);
    grpc::Status status = stub_->DismissGroup(ctx.get(), req, &resp);
    if (!status.ok()) {
        if (out_error) *out_error = status.error_message();
        return false;
    }
    if (resp.code() != 0 && out_error)
        *out_error = resp.message().empty() ? "dismiss failed" : resp.message();
    return resp.code() == 0;
}

bool GroupRpcClient::GetGroupInfo(const std::string& group_id, GroupInfoResult* out_info,
                                  std::string* out_error) {
    if (!stub_) return false;
    swift::group::GetGroupInfoRequest req;
    req.set_group_id(group_id);
    swift::group::GroupInfoResponse resp;
    auto ctx = CreateContext(5000);
    grpc::Status status = stub_->GetGroupInfo(ctx.get(), req, &resp);
    if (!status.ok()) {
        if (out_error) *out_error = status.error_message();
        return false;
    }
    if (resp.code() != 0 && out_error)
        *out_error = resp.message().empty() ? "get group info failed" : resp.message();
    if (resp.code() != 0 || !out_info) return resp.code() == 0;
    const auto& g = resp.group();
    out_info->group_id = g.group_id();
    out_info->group_name = g.group_name();
    out_info->avatar_url = g.avatar_url();
    out_info->owner_id = g.owner_id();
    out_info->member_count = g.member_count();
    out_info->announcement = g.announcement();
    out_info->created_at = g.created_at();
    return true;
}

bool GroupRpcClient::GetGroupMembers(const std::string& group_id, int32_t page, int32_t page_size,
                                    std::vector<GroupMemberResult>* out_members, int32_t* total,
                                    std::string* out_error) {
    if (!stub_) return false;
    swift::group::GetGroupMembersRequest req;
    req.set_group_id(group_id);
    if (page > 0) req.set_page(page);
    if (page_size > 0) req.set_page_size(page_size);
    swift::group::GroupMembersResponse resp;
    auto ctx = CreateContext(5000);
    grpc::Status status = stub_->GetGroupMembers(ctx.get(), req, &resp);
    if (!status.ok()) {
        if (out_error) *out_error = status.error_message();
        return false;
    }
    if (resp.code() != 0 && out_error)
        *out_error = resp.message().empty() ? "get members failed" : resp.message();
    if (resp.code() != 0) return false;
    if (out_members) {
        out_members->clear();
        for (const auto& m : resp.members()) {
            GroupMemberResult r;
            r.user_id = m.user_id();
            r.role = m.role();
            r.nickname = m.nickname();
            r.joined_at = m.joined_at();
            out_members->push_back(r);
        }
    }
    if (total) *total = resp.total();
    return true;
}

bool GroupRpcClient::InviteMembers(const std::string& group_id, const std::string& inviter_id,
                                   const std::vector<std::string>& member_ids, std::string* out_error) {
    if (!stub_) return false;
    swift::group::InviteMembersRequest req;
    req.set_group_id(group_id);
    req.set_inviter_id(inviter_id);
    for (const auto& id : member_ids) req.add_member_ids(id);
    swift::common::CommonResponse resp;
    auto ctx = CreateContext(5000);
    grpc::Status status = stub_->InviteMembers(ctx.get(), req, &resp);
    if (!status.ok()) {
        if (out_error) *out_error = status.error_message();
        return false;
    }
    if (resp.code() != 0 && out_error)
        *out_error = resp.message().empty() ? "invite failed" : resp.message();
    return resp.code() == 0;
}

bool GroupRpcClient::RemoveMember(const std::string& group_id, const std::string& operator_id,
                                  const std::string& member_id, std::string* out_error) {
    if (!stub_) return false;
    swift::group::RemoveMemberRequest req;
    req.set_group_id(group_id);
    req.set_operator_id(operator_id);
    req.set_member_id(member_id);
    swift::common::CommonResponse resp;
    auto ctx = CreateContext(5000);
    grpc::Status status = stub_->RemoveMember(ctx.get(), req, &resp);
    if (!status.ok()) {
        if (out_error) *out_error = status.error_message();
        return false;
    }
    if (resp.code() != 0 && out_error)
        *out_error = resp.message().empty() ? "remove member failed" : resp.message();
    return resp.code() == 0;
}

bool GroupRpcClient::LeaveGroup(const std::string& group_id, const std::string& user_id,
                                std::string* out_error) {
    if (!stub_) return false;
    swift::group::LeaveGroupRequest req;
    req.set_group_id(group_id);
    req.set_user_id(user_id);
    swift::common::CommonResponse resp;
    auto ctx = CreateContext(5000);
    grpc::Status status = stub_->LeaveGroup(ctx.get(), req, &resp);
    if (!status.ok()) {
        if (out_error) *out_error = status.error_message();
        return false;
    }
    if (resp.code() != 0 && out_error)
        *out_error = resp.message().empty() ? "leave failed" : resp.message();
    return resp.code() == 0;
}

bool GroupRpcClient::TransferOwner(const std::string& group_id, const std::string& old_owner_id,
                                   const std::string& new_owner_id, std::string* out_error) {
    if (!stub_) return false;
    swift::group::TransferOwnerRequest req;
    req.set_group_id(group_id);
    req.set_old_owner_id(old_owner_id);
    req.set_new_owner_id(new_owner_id);
    swift::common::CommonResponse resp;
    auto ctx = CreateContext(5000);
    grpc::Status status = stub_->TransferOwner(ctx.get(), req, &resp);
    if (!status.ok()) {
        if (out_error) *out_error = status.error_message();
        return false;
    }
    if (resp.code() != 0 && out_error)
        *out_error = resp.message().empty() ? "transfer owner failed" : resp.message();
    return resp.code() == 0;
}

}  // namespace zone
}  // namespace swift
