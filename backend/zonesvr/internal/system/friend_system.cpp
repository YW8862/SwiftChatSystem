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
    if (!rpc_client_->Connect(config_->friend_svr_addr, !config_->standalone)) return false;
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
                             const std::string& remark,
                             const std::string& token) {
    if (!rpc_client_) return false;
    std::string err;
    return rpc_client_->AddFriend(user_id, friend_id, remark, "", &err, token);
}

bool FriendSystem::HandleFriendRequest(const std::string& user_id,
                                       const std::string& request_id,
                                       bool accept,
                                       const std::string& token) {
    if (!rpc_client_) return false;
    std::string err;
    return rpc_client_->HandleFriendRequest(user_id, request_id, accept, "", &err, token);
}

bool FriendSystem::RemoveFriend(const std::string& user_id, const std::string& friend_id,
                                const std::string& token) {
    if (!rpc_client_) return false;
    std::string err;
    return rpc_client_->RemoveFriend(user_id, friend_id, &err, token);
}

bool FriendSystem::GetFriends(const std::string& user_id, const std::string& group_id,
                              std::vector<FriendInfoResult>* out_friends, std::string* out_error,
                              const std::string& token) {
    if (!rpc_client_) {
        if (out_error) *out_error = "FriendSystem not available";
        return false;
    }
    return rpc_client_->GetFriends(user_id, group_id, out_friends, out_error, token);
}

bool FriendSystem::GetFriendRequests(const std::string& user_id, int32_t type,
                                     std::vector<FriendRequestInfoResult>* out_requests,
                                     std::string* out_error, const std::string& token) {
    if (!rpc_client_) {
        if (out_error) *out_error = "FriendSystem not available";
        return false;
    }
    return rpc_client_->GetFriendRequests(user_id, type, out_requests, out_error, token);
}

bool FriendSystem::BlockUser(const std::string& user_id, const std::string& target_id,
                              const std::string& token) {
    if (!rpc_client_) return false;
    std::string err;
    return rpc_client_->BlockUser(user_id, target_id, &err, token);
}

bool FriendSystem::UnblockUser(const std::string& user_id, const std::string& target_id,
                               const std::string& token) {
    if (!rpc_client_) return false;
    std::string err;
    return rpc_client_->UnblockUser(user_id, target_id, &err, token);
}

bool FriendSystem::IsFriend(const std::string& user_id, const std::string& friend_id) {
    if (!rpc_client_ || friend_id.empty()) return false;
    std::vector<FriendInfoResult> friends;
    std::string err;
    if (!rpc_client_->GetFriends(user_id, "", &friends, &err)) return false;
    for (const auto& f : friends) {
        if (f.friend_id == friend_id) return true;
    }
    return false;
}

bool FriendSystem::IsBlocked(const std::string& user_id, const std::string& target_id) {
    if (!rpc_client_ || target_id.empty()) return false;
    std::vector<std::string> blocked_ids;
    std::string err;
    if (!rpc_client_->GetBlockList(user_id, &blocked_ids, &err)) return false;
    for (const auto& id : blocked_ids) {
        if (id == target_id) return true;
    }
    return false;
}

}  // namespace zone
}  // namespace swift
