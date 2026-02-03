#pragma once

#include <memory>
#include "friend.grpc.pb.h"

namespace swift::friend_ {

class FriendService;  // 业务逻辑类，与 proto 生成的 swift::relation::FriendService 区分

/**
 * 对外 API 层（Handler）
 * 实现 proto 定义的 gRPC FriendService 接口：好友请求、好友列表、分组、黑名单等。
 */
class FriendHandler : public ::swift::relation::FriendService::Service {
public:
    explicit FriendHandler(std::shared_ptr<FriendService> service);
    ~FriendHandler() override;

    ::grpc::Status AddFriend(::grpc::ServerContext* context,
                             const ::swift::relation::AddFriendRequest* request,
                             ::swift::common::CommonResponse* response) override;

    ::grpc::Status HandleFriendRequest(::grpc::ServerContext* context,
                                       const ::swift::relation::HandleFriendReq* request,
                                       ::swift::common::CommonResponse* response) override;

    ::grpc::Status RemoveFriend(::grpc::ServerContext* context,
                                const ::swift::relation::RemoveFriendRequest* request,
                                ::swift::common::CommonResponse* response) override;

    ::grpc::Status GetFriends(::grpc::ServerContext* context,
                              const ::swift::relation::GetFriendsRequest* request,
                              ::swift::relation::FriendListResponse* response) override;

    ::grpc::Status BlockUser(::grpc::ServerContext* context,
                             const ::swift::relation::BlockUserRequest* request,
                             ::swift::common::CommonResponse* response) override;

    ::grpc::Status UnblockUser(::grpc::ServerContext* context,
                               const ::swift::relation::UnblockUserRequest* request,
                               ::swift::common::CommonResponse* response) override;

    ::grpc::Status GetBlockList(::grpc::ServerContext* context,
                                const ::swift::relation::GetBlockListRequest* request,
                                ::swift::relation::BlockListResponse* response) override;

    ::grpc::Status CreateFriendGroup(::grpc::ServerContext* context,
                                     const ::swift::relation::CreateFriendGroupRequest* request,
                                     ::swift::common::CommonResponse* response) override;

    ::grpc::Status GetFriendGroups(::grpc::ServerContext* context,
                                   const ::swift::relation::GetFriendGroupsRequest* request,
                                   ::swift::relation::FriendGroupListResponse* response) override;

    ::grpc::Status DeleteFriendGroup(::grpc::ServerContext* context,
                                     const ::swift::relation::DeleteFriendGroupRequest* request,
                                     ::swift::common::CommonResponse* response) override;

    ::grpc::Status MoveFriend(::grpc::ServerContext* context,
                              const ::swift::relation::MoveFriendRequest* request,
                              ::swift::common::CommonResponse* response) override;

    ::grpc::Status SetRemark(::grpc::ServerContext* context,
                             const ::swift::relation::SetRemarkRequest* request,
                             ::swift::common::CommonResponse* response) override;

    ::grpc::Status GetFriendRequests(::grpc::ServerContext* context,
                                     const ::swift::relation::GetFriendRequestsRequest* request,
                                     ::swift::relation::FriendRequestListResponse* response) override;

private:
    std::shared_ptr<FriendService> service_;
};

}  // namespace swift::friend_
