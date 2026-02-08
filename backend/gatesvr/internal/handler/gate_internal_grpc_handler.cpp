/**
 * @file gate_internal_grpc_handler.cpp
 * @brief GateInternalService gRPC 实现
 */

#include "gate_internal_grpc_handler.h"
#include "service/gate_service.h"
#include "swift/error_code.h"
#include <string>

namespace swift::gate {

GateInternalGrpcHandler::GateInternalGrpcHandler(std::shared_ptr<GateService> service)
    : service_(std::move(service)) {}

GateInternalGrpcHandler::~GateInternalGrpcHandler() = default;

::grpc::Status GateInternalGrpcHandler::PushMessage(::grpc::ServerContext* context,
                                                    const ::swift::gate::PushMessageRequest* request,
                                                    ::swift::common::CommonResponse* response) {
    (void)context;
    if (!request || !response)
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "null request/response");
    std::string payload(request->payload().begin(), request->payload().end());
    bool ok = service_->PushToUser(request->user_id(),
                                   request->cmd().empty() ? "message" : request->cmd(),
                                   payload);
    response->set_code(ok ? swift::ErrorCodeToInt(swift::ErrorCode::OK)
                         : swift::ErrorCodeToInt(swift::ErrorCode::USER_OFFLINE));
    if (!ok)
        response->set_message(swift::ErrorCodeToString(swift::ErrorCode::USER_OFFLINE));
    return ::grpc::Status::OK;
}

::grpc::Status GateInternalGrpcHandler::DisconnectUser(::grpc::ServerContext* context,
                                                       const ::swift::gate::DisconnectUserRequest* request,
                                                       ::swift::common::CommonResponse* response) {
    (void)context;
    if (!request || !response)
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "null request/response");
    std::string user_id = request->user_id();
    std::string conn_id = service_->GetConnIdByUser(user_id);
    if (!conn_id.empty())
        service_->CloseConnection(conn_id);
    response->set_code(swift::ErrorCodeToInt(swift::ErrorCode::OK));
    return ::grpc::Status::OK;
}

}  // namespace swift::gate
