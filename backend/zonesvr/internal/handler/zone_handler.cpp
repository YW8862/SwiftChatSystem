/**
 * @file zone_handler.cpp
 * @brief ZoneService gRPC 服务实现：从 request 解析参数，调用 ZoneService，写回 response。
 */

#include "zone_handler.h"
#include "../service/zone_service.h"
#include <swift/error_code.h>

namespace swift::zone {

ZoneHandler::ZoneHandler(std::shared_ptr<ZoneServiceImpl> service)
    : service_(std::move(service)) {}

ZoneHandler::~ZoneHandler() = default;

::grpc::Status ZoneHandler::UserOnline(::grpc::ServerContext* context,
                                       const ::swift::zone::UserOnlineRequest* request,
                                       ::swift::common::CommonResponse* response) {
    (void)context;
    if (!request || !response) return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "null request/response");
    bool ok = service_->UserOnline(
        request->user_id(),
        request->gate_id(),
        request->device_type(),
        request->device_id());
    response->set_code(ok ? swift::ErrorCodeToInt(swift::ErrorCode::OK)
                         : swift::ErrorCodeToInt(swift::ErrorCode::SERVICE_UNAVAILABLE));
    if (!ok) response->set_message(swift::ErrorCodeToString(swift::ErrorCode::SERVICE_UNAVAILABLE));
    return ::grpc::Status::OK;
}

::grpc::Status ZoneHandler::UserOffline(::grpc::ServerContext* context,
                                        const ::swift::zone::UserOfflineRequest* request,
                                        ::swift::common::CommonResponse* response) {
    (void)context;
    if (!request || !response) return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "null request/response");
    bool ok = service_->UserOffline(request->user_id(), request->gate_id());
    response->set_code(ok ? swift::ErrorCodeToInt(swift::ErrorCode::OK)
                         : swift::ErrorCodeToInt(swift::ErrorCode::INTERNAL_ERROR));
    if (!ok) response->set_message(swift::ErrorCodeToString(swift::ErrorCode::INTERNAL_ERROR));
    return ::grpc::Status::OK;
}

::grpc::Status ZoneHandler::RouteMessage(::grpc::ServerContext* context,
                                         const ::swift::zone::RouteMessageRequest* request,
                                         ::swift::zone::RouteMessageResponse* response) {
    (void)context;
    if (!request || !response) return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "null request/response");
    std::string payload(request->payload().begin(), request->payload().end());
    auto result = service_->RouteToUser(
        request->to_user_id(),
        request->msg_type().empty() ? "message" : request->msg_type(),
        payload);
    response->set_code(swift::ErrorCodeToInt(swift::ErrorCode::OK));
    response->set_delivered(result.delivered);
    return ::grpc::Status::OK;
}

::grpc::Status ZoneHandler::Broadcast(::grpc::ServerContext* context,
                                      const ::swift::zone::BroadcastRequest* request,
                                      ::swift::zone::BroadcastResponse* response) {
    (void)context;
    if (!request || !response) return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "null request/response");
    std::vector<std::string> user_ids(request->user_ids().begin(), request->user_ids().end());
    std::string payload(request->payload().begin(), request->payload().end());
    std::string msg_type = request->msg_type().empty() ? "broadcast" : request->msg_type();
    auto result = service_->Broadcast(user_ids, msg_type, payload);
    response->set_code(swift::ErrorCodeToInt(swift::ErrorCode::OK));
    response->set_online_count(result.online_count);
    response->set_delivered_count(result.delivered_count);
    return ::grpc::Status::OK;
}

::grpc::Status ZoneHandler::GetUserStatus(::grpc::ServerContext* context,
                                         const ::swift::zone::GetUserStatusRequest* request,
                                         ::swift::zone::GetUserStatusResponse* response) {
    (void)context;
    if (!request || !response) return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "null request/response");
    for (const auto& uid : request->user_ids()) {
        auto* s = response->add_statuses();
        s->set_user_id(uid);
        auto opt = service_->GetUserSession(uid);
        if (opt) {
            s->set_online(true);
            s->set_gate_id(opt->gate_id);
            s->set_device_type(opt->device_type);
            s->set_last_active_at(opt->last_active_at);
        } else {
            s->set_online(false);
        }
    }
    response->set_code(swift::ErrorCodeToInt(swift::ErrorCode::OK));
    return ::grpc::Status::OK;
}

::grpc::Status ZoneHandler::PushToUser(::grpc::ServerContext* context,
                                       const ::swift::zone::PushToUserRequest* request,
                                       ::swift::common::CommonResponse* response) {
    (void)context;
    if (!request || !response) return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "null request/response");
    std::string payload(request->payload().begin(), request->payload().end());
    auto result = service_->RouteToUser(
        request->user_id(),
        request->cmd().empty() ? "push" : request->cmd(),
        payload);
    if (!result.user_online) {
        response->set_code(swift::ErrorCodeToInt(swift::ErrorCode::USER_OFFLINE));
        response->set_message(swift::ErrorCodeToString(swift::ErrorCode::USER_OFFLINE));
    } else if (!result.delivered) {
        response->set_code(swift::ErrorCodeToInt(swift::ErrorCode::FORWARD_FAILED));
        response->set_message(swift::ErrorCodeToString(swift::ErrorCode::FORWARD_FAILED));
    } else {
        response->set_code(swift::ErrorCodeToInt(swift::ErrorCode::OK));
    }
    return ::grpc::Status::OK;
}

::grpc::Status ZoneHandler::KickUser(::grpc::ServerContext* context,
                                     const ::swift::zone::KickUserRequest* request,
                                     ::swift::common::CommonResponse* response) {
    (void)context;
    if (!request || !response) return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "null request/response");
    bool ok = service_->KickUser(request->user_id(), request->reason());
    response->set_code(ok ? swift::ErrorCodeToInt(swift::ErrorCode::OK)
                         : swift::ErrorCodeToInt(swift::ErrorCode::FORWARD_FAILED));
    if (!ok) response->set_message(swift::ErrorCodeToString(swift::ErrorCode::FORWARD_FAILED));
    return ::grpc::Status::OK;
}

::grpc::Status ZoneHandler::GateRegister(::grpc::ServerContext* context,
                                         const ::swift::zone::GateRegisterRequest* request,
                                         ::swift::common::CommonResponse* response) {
    (void)context;
    if (!request || !response) return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "null request/response");
    bool ok = service_->RegisterGate(request->gate_id(), request->address());
    response->set_code(ok ? swift::ErrorCodeToInt(swift::ErrorCode::OK)
                         : swift::ErrorCodeToInt(swift::ErrorCode::INTERNAL_ERROR));
    if (!ok) response->set_message(swift::ErrorCodeToString(swift::ErrorCode::INTERNAL_ERROR));
    return ::grpc::Status::OK;
}

::grpc::Status ZoneHandler::GateHeartbeat(::grpc::ServerContext* context,
                                          const ::swift::zone::GateHeartbeatRequest* request,
                                          ::swift::common::CommonResponse* response) {
    (void)context;
    if (!request || !response) return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "null request/response");
    bool ok = service_->GateHeartbeat(request->gate_id(), request->current_connections());
    response->set_code(ok ? swift::ErrorCodeToInt(swift::ErrorCode::OK)
                         : swift::ErrorCodeToInt(swift::ErrorCode::GATE_NOT_FOUND));
    if (!ok) response->set_message(swift::ErrorCodeToString(swift::ErrorCode::GATE_NOT_FOUND));
    return ::grpc::Status::OK;
}

::grpc::Status ZoneHandler::HandleClientRequest(::grpc::ServerContext* context,
                                                const ::swift::zone::HandleClientRequestRequest* request,
                                                ::swift::zone::HandleClientRequestResponse* response) {
    (void)context;
    if (!request || !response) return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "null request/response");
    std::string payload(request->payload().begin(), request->payload().end());
    auto result = service_->HandleClientRequest(
        request->conn_id(),
        request->user_id(),
        request->cmd(),
        payload,
        request->request_id(),
        request->token());
    response->set_code(result.code);
    if (!result.message.empty()) response->set_message(result.message);
    if (!result.payload.empty()) response->set_payload(result.payload);
    if (!result.request_id.empty()) response->set_request_id(result.request_id);
    return ::grpc::Status::OK;
}

}  // namespace swift::zone
