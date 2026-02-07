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
    if (!rpc_client_->Connect(config_->chat_svr_addr)) return false;
    rpc_client_->InitStub();
    return true;
}

void GroupSystem::Shutdown() {
    if (rpc_client_) {
        rpc_client_.reset();
    }
}

std::string GroupSystem::CreateGroup(const std::string& creator_id,
                                     const std::string& group_name,
                                     const std::vector<std::string>& member_ids) {
    // TODO:
    // 1. 调用 GroupService.CreateGroup
    // 2. 通知所有初始成员
    return "";
}

bool GroupSystem::DismissGroup(const std::string& group_id, const std::string& operator_id) {
    // TODO:
    // 1. 调用 GroupService.DismissGroup
    // 2. 通知所有成员群已解散
    return false;
}

bool GroupSystem::InviteMembers(const std::string& group_id,
                                 const std::string& inviter_id,
                                 const std::vector<std::string>& member_ids) {
    // TODO:
    // 1. 调用 GroupService.InviteMembers
    // 2. 通知被邀请的成员
    return false;
}

bool GroupSystem::RemoveMember(const std::string& group_id,
                                const std::string& operator_id,
                                const std::string& member_id) {
    // TODO:
    // 1. 调用 GroupService.RemoveMember
    // 2. 通知被移除的成员和群内其他成员
    return false;
}

bool GroupSystem::LeaveGroup(const std::string& group_id, const std::string& user_id) {
    // TODO:
    // 1. 调用 GroupService.LeaveGroup
    // 2. 通知群内其他成员
    return false;
}

bool GroupSystem::TransferOwner(const std::string& group_id,
                                 const std::string& old_owner_id,
                                 const std::string& new_owner_id) {
    // TODO:
    // 1. 调用 GroupService.TransferOwner
    // 2. 通知群内所有成员
    return false;
}

void GroupSystem::BroadcastToGroupMembers(const std::string& group_id, const std::string& payload) {
    // TODO:
    // 1. 获取群成员列表
    // 2. 从 SessionStore 获取在线成员
    // 3. 路由消息到各自的 Gate
}

}  // namespace zone
}  // namespace swift
