#include "online_handler.h"
#include "../service/online_service.h"
#include "swift/error_code.h"

namespace swift::online {

OnlineHandler::OnlineHandler(std::shared_ptr<OnlineServiceCore> service)
    : service_(std::move(service)) {}

OnlineHandler::~OnlineHandler() = default;

::grpc::Status OnlineHandler::Login(::grpc::ServerContext* context,
                                    const ::swift::online::LoginRequest* request,
                                    ::swift::online::LoginResponse* response) {
    (void)context;
    if (!request || !response)
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "null request/response");

    auto result = service_->Login(request->user_id(), request->device_id(), request->device_type());
    response->set_code(result.success ? swift::ErrorCodeToInt(swift::ErrorCode::OK)
                                      : swift::ErrorCodeToInt(swift::ErrorCode::AUTH_FAILED));
    if (!result.error.empty())
        response->set_message(result.error);
    if (result.success) {
        response->set_token(result.token);
        response->set_expire_at(result.expire_at);
    }
    return ::grpc::Status::OK;
}

::grpc::Status OnlineHandler::Logout(::grpc::ServerContext* context,
                                      const ::swift::online::LogoutRequest* request,
                                      ::swift::common::CommonResponse* response) {
    (void)context;
    if (!request || !response)
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "null request/response");

    auto result = service_->Logout(request->user_id(), request->token());
    response->set_code(result.success ? swift::ErrorCodeToInt(swift::ErrorCode::OK)
                                      : swift::ErrorCodeToInt(swift::ErrorCode::AUTH_FAILED));
    if (!result.error.empty())
        response->set_message(result.error);
    return ::grpc::Status::OK;
}

::grpc::Status OnlineHandler::ValidateToken(::grpc::ServerContext* context,
                                            const ::swift::online::TokenRequest* request,
                                            ::swift::online::TokenResponse* response) {
    (void)context;
    if (!request || !response)
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "null request/response");

    auto result = service_->ValidateToken(request->token());
    response->set_code(result.valid ? swift::ErrorCodeToInt(swift::ErrorCode::OK)
                                    : swift::ErrorCodeToInt(swift::ErrorCode::TOKEN_INVALID));
    if (!result.valid)
        response->set_message(swift::ErrorCodeToString(swift::ErrorCode::TOKEN_INVALID));
    else {
        response->set_user_id(result.user_id);
        response->set_valid(true);
    }
    return ::grpc::Status::OK;
}

}  // namespace swift::online
