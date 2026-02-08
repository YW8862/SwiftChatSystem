/**
 * @file friend_rpc_client.cpp
 * @brief FriendRpcClient 实现
 */

#include "friend_rpc_client.h"

namespace swift {
namespace zone {

void FriendRpcClient::InitStub() {
    stub_ = swift::relation::FriendService::NewStub(GetChannel());
}

bool FriendRpcClient::AddFriend(const std::string& user_id, const std::string& friend_id,
                               const std::string& remark, const std::string& group_id,
                               std::string* out_error, const std::string& token) {
    if (!stub_) return false;
    swift::relation::AddFriendRequest req;
    req.set_user_id(user_id);
    req.set_friend_id(friend_id);
    if (!remark.empty()) req.set_remark(remark);
    if (!group_id.empty()) req.set_group_id(group_id);
    swift::common::CommonResponse resp;
    auto ctx = CreateContext(5000, token);
    grpc::Status status = stub_->AddFriend(ctx.get(), req, &resp);
    if (!status.ok()) {
        if (out_error) *out_error = status.error_message();
        return false;
    }
    if (resp.code() != 0 && out_error)
        *out_error = resp.message().empty() ? "add friend failed" : resp.message();
    return resp.code() == 0;
}

bool FriendRpcClient::HandleFriendRequest(const std::string& user_id, const std::string& request_id,
                                          bool accept, const std::string& group_id,
                                          std::string* out_error, const std::string& token) {
    if (!stub_) return false;
    swift::relation::HandleFriendReq req;
    req.set_user_id(user_id);
    req.set_request_id(request_id);
    req.set_accept(accept);
    if (!group_id.empty()) req.set_group_id(group_id);
    swift::common::CommonResponse resp;
    auto ctx = CreateContext(5000, token);
    grpc::Status status = stub_->HandleFriendRequest(ctx.get(), req, &resp);
    if (!status.ok()) {
        if (out_error) *out_error = status.error_message();
        return false;
    }
    if (resp.code() != 0 && out_error)
        *out_error = resp.message().empty() ? "handle request failed" : resp.message();
    return resp.code() == 0;
}

bool FriendRpcClient::RemoveFriend(const std::string& user_id, const std::string& friend_id,
                                  std::string* out_error, const std::string& token) {
    if (!stub_) return false;
    swift::relation::RemoveFriendRequest req;
    req.set_user_id(user_id);
    req.set_friend_id(friend_id);
    swift::common::CommonResponse resp;
    auto ctx = CreateContext(5000, token);
    grpc::Status status = stub_->RemoveFriend(ctx.get(), req, &resp);
    if (!status.ok()) {
        if (out_error) *out_error = status.error_message();
        return false;
    }
    if (resp.code() != 0 && out_error)
        *out_error = resp.message().empty() ? "remove friend failed" : resp.message();
    return resp.code() == 0;
}

bool FriendRpcClient::GetFriends(const std::string& user_id, const std::string& group_id,
                                 std::vector<FriendInfoResult>* out_friends, std::string* out_error,
                                 const std::string& token) {
    if (!stub_) return false;
    swift::relation::GetFriendsRequest req;
    req.set_user_id(user_id);
    if (!group_id.empty()) req.set_group_id(group_id);
    swift::relation::FriendListResponse resp;
    auto ctx = CreateContext(5000, token);
    grpc::Status status = stub_->GetFriends(ctx.get(), req, &resp);
    if (!status.ok()) {
        if (out_error) *out_error = status.error_message();
        return false;
    }
    if (resp.code() != 0 && out_error)
        *out_error = resp.message().empty() ? "get friends failed" : resp.message();
    if (resp.code() != 0) return false;
    if (out_friends) {
        out_friends->clear();
        for (const auto& f : resp.friends()) {
            FriendInfoResult r;
            r.friend_id = f.friend_id();
            r.remark = f.remark();
            r.group_id = f.group_id();
            r.added_at = f.added_at();
            if (f.has_profile()) {
                r.nickname = f.profile().nickname();
                r.avatar_url = f.profile().avatar_url();
            }
            out_friends->push_back(r);
        }
    }
    return true;
}

bool FriendRpcClient::BlockUser(const std::string& user_id, const std::string& target_id,
                                std::string* out_error, const std::string& token) {
    if (!stub_) return false;
    swift::relation::BlockUserRequest req;
    req.set_user_id(user_id);
    req.set_target_id(target_id);
    swift::common::CommonResponse resp;
    auto ctx = CreateContext(5000, token);
    grpc::Status status = stub_->BlockUser(ctx.get(), req, &resp);
    if (!status.ok()) {
        if (out_error) *out_error = status.error_message();
        return false;
    }
    if (resp.code() != 0 && out_error)
        *out_error = resp.message().empty() ? "block failed" : resp.message();
    return resp.code() == 0;
}

bool FriendRpcClient::UnblockUser(const std::string& user_id, const std::string& target_id,
                                  std::string* out_error, const std::string& token) {
    if (!stub_) return false;
    swift::relation::UnblockUserRequest req;
    req.set_user_id(user_id);
    req.set_target_id(target_id);
    swift::common::CommonResponse resp;
    auto ctx = CreateContext(5000, token);
    grpc::Status status = stub_->UnblockUser(ctx.get(), req, &resp);
    if (!status.ok()) {
        if (out_error) *out_error = status.error_message();
        return false;
    }
    if (resp.code() != 0 && out_error)
        *out_error = resp.message().empty() ? "unblock failed" : resp.message();
    return resp.code() == 0;
}

}  // namespace zone
}  // namespace swift
