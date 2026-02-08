/**
 * @file friend_system.cpp
 * @brief FriendSystem 实现
 */

#include "friend_system.h"
#include "../rpc/friend_rpc_client.h"
#include "../config/config.h"

namespace swift {
namespace zone {

FriendSystem::FriendSystem() = default;
FriendSystem::~FriendSystem() = default;

bool FriendSystem::Init() {
    if (!config_) return true;
    rpc_client_ = std::make_unique<FriendRpcClient>();
    if (!rpc_client_->Connect(config_->friend_svr_addr)) return false;
    rpc_client_->InitStub();
    return true;
}

void FriendSystem::Shutdown() {
    if (rpc_client_) {
        rpc_client_->Disconnect();
        rpc_client_.reset();
    }
}

bool FriendSystem::AddFriend(const std::string& user_id,
                             const std::string& friend_id,
                             const std::string& remark) {
    if (!rpc_client_) return false;
    std::string err;
    return rpc_client_->AddFriend(user_id, friend_id, remark, "", &err);
}

bool FriendSystem::HandleFriendRequest(const std::string& user_id,
                                       const std::string& request_id,
                                       bool accept) {
    if (!rpc_client_) return false;
    std::string err;
    return rpc_client_->HandleFriendRequest(user_id, request_id, accept, "", &err);
}

bool FriendSystem::RemoveFriend(const std::string& user_id, const std::string& friend_id) {
    if (!rpc_client_) return false;
    std::string err;
    return rpc_client_->RemoveFriend(user_id, friend_id, &err);
}

bool FriendSystem::BlockUser(const std::string& user_id, const std::string& target_id) {
    if (!rpc_client_) return false;
    std::string err;
    return rpc_client_->BlockUser(user_id, target_id, &err);
}

bool FriendSystem::UnblockUser(const std::string& user_id, const std::string& target_id) {
    if (!rpc_client_) return false;
    std::string err;
    return rpc_client_->UnblockUser(user_id, target_id, &err);
}

bool FriendSystem::IsFriend(const std::string& user_id, const std::string& friend_id) {
    // TODO: 调用 FriendSvr 或使用缓存
    return false;
}

bool FriendSystem::IsBlocked(const std::string& user_id, const std::string& target_id) {
    // TODO: 调用 FriendSvr 或使用缓存
    return false;
}

}  // namespace zone
}  // namespace swift
