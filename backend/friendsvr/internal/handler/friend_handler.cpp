/**
 * @file friend_handler.cpp
 * @brief 好友 gRPC 对外 API：将请求转调 FriendService，结果写入 proto 响应。
 */

#include "friend_handler.h"
#include "../service/friend_service.h"
#include "swift/error_code.h"

namespace swift::friend_ {

FriendHandler::FriendHandler(std::shared_ptr<FriendService> service)
    : service_(std::move(service)) {}

FriendHandler::~FriendHandler() = default;

namespace {

void SetOk(::swift::common::CommonResponse *response) {
  response->set_code(static_cast<int>(swift::ErrorCode::OK));
}

void SetFail(::swift::common::CommonResponse *response,
             swift::ErrorCode code = swift::ErrorCode::UNKNOWN,
             const std::string &message = "operation failed") {
  response->set_code(swift::ErrorCodeToInt(code));
  response->set_message(message);
}

} // namespace

// ============================================================================
// 好友请求
// ============================================================================

::grpc::Status
FriendHandler::AddFriend(::grpc::ServerContext *context,
                         const ::swift::relation::AddFriendRequest *request,
                         ::swift::common::CommonResponse *response) {
  (void)context;
  bool ok = service_->AddFriend(request->user_id(), request->friend_id(),
                                request->remark());
  if (ok)
    SetOk(response);
  else
    SetFail(response);
  return ::grpc::Status::OK;
}

::grpc::Status FriendHandler::HandleFriendRequest(
    ::grpc::ServerContext *context,
    const ::swift::relation::HandleFriendReq *request,
    ::swift::common::CommonResponse *response) {
  (void)context;
  bool ok = service_->HandleRequest(request->user_id(), request->request_id(),
                                    request->accept(), request->group_id());
  if (ok)
    SetOk(response);
  else
    SetFail(response);
  return ::grpc::Status::OK;
}

// ============================================================================
// 好友关系
// ============================================================================

::grpc::Status FriendHandler::RemoveFriend(
    ::grpc::ServerContext *context,
    const ::swift::relation::RemoveFriendRequest *request,
    ::swift::common::CommonResponse *response) {
  (void)context;
  bool ok = service_->RemoveFriend(request->user_id(), request->friend_id());
  if (ok)
    SetOk(response);
  else
    SetFail(response, swift::ErrorCode::FRIEND_NOT_FOUND, "friend not found");
  return ::grpc::Status::OK;
}

::grpc::Status
FriendHandler::GetFriends(::grpc::ServerContext *context,
                          const ::swift::relation::GetFriendsRequest *request,
                          ::swift::relation::FriendListResponse *response) {
  (void)context;
  auto list = service_->GetFriends(request->user_id(), request->group_id());
  response->set_code(static_cast<int>(swift::ErrorCode::OK));
  for (const auto &f : list) {
    auto *info = response->add_friends();
    info->set_friend_id(f.friend_id);
    info->set_remark(f.remark);
    info->set_group_id(f.group_id);
    info->set_status(0); // 在线状态由上层/网关填充
    info->set_added_at(f.added_at);
    // profile 需查 AuthSvr 或缓存，此处留空
  }
  return ::grpc::Status::OK;
}

// ============================================================================
// 黑名单
// ============================================================================

::grpc::Status
FriendHandler::BlockUser(::grpc::ServerContext *context,
                         const ::swift::relation::BlockUserRequest *request,
                         ::swift::common::CommonResponse *response) {
  (void)context;
  bool ok = service_->Block(request->user_id(), request->target_id());
  if (ok)
    SetOk(response);
  else
    SetFail(response);
  return ::grpc::Status::OK;
}

::grpc::Status
FriendHandler::UnblockUser(::grpc::ServerContext *context,
                           const ::swift::relation::UnblockUserRequest *request,
                           ::swift::common::CommonResponse *response) {
  (void)context;
  bool ok = service_->Unblock(request->user_id(), request->target_id());
  if (ok)
    SetOk(response);
  else
    SetFail(response);
  return ::grpc::Status::OK;
}

::grpc::Status FriendHandler::GetBlockList(
    ::grpc::ServerContext *context,
    const ::swift::relation::GetBlockListRequest *request,
    ::swift::relation::BlockListResponse *response) {
  (void)context;
  auto ids = service_->GetBlockList(request->user_id());
  response->set_code(static_cast<int>(swift::ErrorCode::OK));
  for (const auto &id : ids)
    response->add_blocked_ids(id);
  return ::grpc::Status::OK;
}

// ============================================================================
// 分组
// ============================================================================

::grpc::Status FriendHandler::CreateFriendGroup(
    ::grpc::ServerContext *context,
    const ::swift::relation::CreateFriendGroupRequest *request,
    ::swift::common::CommonResponse *response) {
  (void)context;
  std::string group_id;
  bool ok = service_->CreateFriendGroup(request->user_id(),
                                        request->group_name(), &group_id);
  if (ok)
    SetOk(response);
  else
    SetFail(response);
  return ::grpc::Status::OK;
}

::grpc::Status FriendHandler::GetFriendGroups(
    ::grpc::ServerContext *context,
    const ::swift::relation::GetFriendGroupsRequest *request,
    ::swift::relation::FriendGroupListResponse *response) {
  (void)context;
  auto groups = service_->GetFriendGroups(request->user_id());
  response->set_code(static_cast<int>(swift::ErrorCode::OK));
  for (const auto &g : groups) {
    auto *out = response->add_groups();
    out->set_group_id(g.group_id);
    out->set_group_name(g.group_name);
    out->set_sort_order(g.sort_order);
    auto friends_in_group =
        service_->GetFriends(request->user_id(), g.group_id);
    out->set_friend_count(static_cast<int32_t>(friends_in_group.size()));
  }
  return ::grpc::Status::OK;
}

::grpc::Status FriendHandler::DeleteFriendGroup(
    ::grpc::ServerContext *context,
    const ::swift::relation::DeleteFriendGroupRequest *request,
    ::swift::common::CommonResponse *response) {
  (void)context;
  swift::ErrorCode code =
      service_->DeleteFriendGroup(request->user_id(), request->group_id());
  if (code == swift::ErrorCode::OK)
    SetOk(response);
  else
    SetFail(response, code);
  return ::grpc::Status::OK;
}

::grpc::Status
FriendHandler::MoveFriend(::grpc::ServerContext *context,
                          const ::swift::relation::MoveFriendRequest *request,
                          ::swift::common::CommonResponse *response) {
  (void)context;
  bool ok = service_->MoveFriend(request->user_id(), request->friend_id(),
                                 request->to_group_id());
  if (ok)
    SetOk(response);
  else
    SetFail(response, swift::ErrorCode::FRIEND_NOT_FOUND, "friend not found");
  return ::grpc::Status::OK;
}

::grpc::Status
FriendHandler::SetRemark(::grpc::ServerContext *context,
                         const ::swift::relation::SetRemarkRequest *request,
                         ::swift::common::CommonResponse *response) {
  (void)context;
  bool ok = service_->SetRemark(request->user_id(), request->friend_id(),
                                request->remark());
  if (ok)
    SetOk(response);
  else
    SetFail(response, swift::ErrorCode::FRIEND_NOT_FOUND, "friend not found");
  return ::grpc::Status::OK;
}

// ============================================================================
// 好友请求列表
// ============================================================================

::grpc::Status FriendHandler::GetFriendRequests(
    ::grpc::ServerContext *context,
    const ::swift::relation::GetFriendRequestsRequest *request,
    ::swift::relation::FriendRequestListResponse *response) {
  (void)context;
  int type = request->type();
  auto list = service_->GetFriendRequests(request->user_id(), type);
  response->set_code(static_cast<int>(swift::ErrorCode::OK));
  for (const auto &r : list) {
    auto *out = response->add_requests();
    out->set_request_id(r.request_id);
    out->set_from_user_id(r.from_user_id);
    out->set_to_user_id(r.to_user_id);
    out->set_remark(r.remark);
    out->set_status(r.status);
    out->set_created_at(r.created_at);
    // from_profile 需查 AuthSvr，此处留空
  }
  return ::grpc::Status::OK;
}

} // namespace swift::friend_
