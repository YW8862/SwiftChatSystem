/**
 * @file group_system.cpp
 * @brief GroupSystem 实现
 */

#include "group_system.h"
#include "../rpc/group_rpc_client.h"
#include "../config/config.h"

namespace swift {
namespace zone {

GroupSystem::GroupSystem() = default;
GroupSystem::~GroupSystem() = default;

bool GroupSystem::Init() {
    if (!config_) return true;
    rpc_client_ = std::make_unique<GroupRpcClient>();
    if (!rpc_client_->Connect(config_->chat_svr_addr, !config_->standalone)) return false;
    rpc_client_->InitStub();
    return true;
}

void GroupSystem::Shutdown() {
    if (rpc_client_) {
        rpc_client_->Disconnect();
        rpc_client_.reset();
    }
}

std::string GroupSystem::CreateGroup(const std::string& creator_id,
                                    const std::string& group_name,
                                    const std::vector<std::string>& member_ids) {
    if (!rpc_client_) return "";
    auto r = rpc_client_->CreateGroup(creator_id, group_name, "", member_ids);
    return r.success ? r.group_id : "";
}

bool GroupSystem::DismissGroup(const std::string& group_id, const std::string& operator_id) {
    if (!rpc_client_) return false;
    std::string err;
    return rpc_client_->DismissGroup(group_id, operator_id, &err);
}

bool GroupSystem::InviteMembers(const std::string& group_id,
                                const std::string& inviter_id,
                                const std::vector<std::string>& member_ids) {
    if (!rpc_client_) return false;
    std::string err;
    return rpc_client_->InviteMembers(group_id, inviter_id, member_ids, &err);
}

bool GroupSystem::RemoveMember(const std::string& group_id,
                               const std::string& operator_id,
                               const std::string& member_id) {
    if (!rpc_client_) return false;
    std::string err;
    return rpc_client_->RemoveMember(group_id, operator_id, member_id, &err);
}

bool GroupSystem::LeaveGroup(const std::string& group_id, const std::string& user_id) {
    if (!rpc_client_) return false;
    std::string err;
    return rpc_client_->LeaveGroup(group_id, user_id, &err);
}

bool GroupSystem::TransferOwner(const std::string& group_id,
                                const std::string& old_owner_id,
                                const std::string& new_owner_id) {
    if (!rpc_client_) return false;
    std::string err;
    return rpc_client_->TransferOwner(group_id, old_owner_id, new_owner_id, &err);
}

bool GroupSystem::GetGroupMembers(const std::string& group_id, int32_t page, int32_t page_size,
                                   std::vector<GroupMemberResult>* out_members, int32_t* total,
                                   std::string* out_error) {
    if (!rpc_client_) {
        if (out_error) *out_error = "GroupSystem not available";
        return false;
    }
    return rpc_client_->GetGroupMembers(group_id, page, page_size, out_members, total, out_error);
}

void GroupSystem::BroadcastToGroupMembers(const std::string& group_id, const std::string& payload) {
    // TODO:
    // 1. 获取群成员列表
    // 2. 从 SessionStore 获取在线成员
    // 3. 路由消息到各自的 Gate
}

}  // namespace zone
}  // namespace swift
