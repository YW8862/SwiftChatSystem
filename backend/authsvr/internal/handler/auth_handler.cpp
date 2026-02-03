#include "auth_handler.h"
#include "../service/auth_service.h"
#include "swift/error_code.h"

namespace swift::auth {

AuthHandler::AuthHandler(std::shared_ptr<AuthServiceCore> service)
    : service_(std::move(service)) {}

AuthHandler::~AuthHandler() = default;

::grpc::Status AuthHandler::Register(::grpc::ServerContext* context,
                                      const ::swift::auth::RegisterRequest* request,
                                      ::swift::auth::RegisterResponse* response) {
    (void)context;

    auto result = service_->Register(
        request->username(),
        request->password(),
        request->nickname(),
        request->email(),
        request->avatar_url());

    if (result.success) {
        response->set_code(static_cast<int>(swift::ErrorCode::OK));
        response->set_user_id(result.user_id);
    } else {
        response->set_code(swift::ErrorCodeToInt(result.error_code));
        response->set_message(result.error);
    }
    return ::grpc::Status::OK;
}

::grpc::Status AuthHandler::VerifyCredentials(::grpc::ServerContext* context,
                                               const ::swift::auth::VerifyCredentialsRequest* request,
                                               ::swift::auth::VerifyCredentialsResponse* response) {
    (void)context;

    auto result = service_->VerifyCredentials(request->username(), request->password());

    if (result.success) {
        response->set_code(static_cast<int>(swift::ErrorCode::OK));
        response->set_user_id(result.user_id);
        if (result.profile) {
            auto* p = response->mutable_profile();
            p->set_user_id(result.profile->user_id);
            p->set_username(result.profile->username);
            p->set_nickname(result.profile->nickname);
            p->set_avatar_url(result.profile->avatar_url);
            p->set_signature(result.profile->signature);
            p->set_gender(result.profile->gender);
            p->set_created_at(result.profile->created_at);
        }
    } else {
        response->set_code(swift::ErrorCodeToInt(result.error_code));
        response->set_message(result.error);
    }
    return ::grpc::Status::OK;
}

::grpc::Status AuthHandler::GetProfile(::grpc::ServerContext* context,
                                        const ::swift::auth::GetProfileRequest* request,
                                        ::swift::auth::UserProfile* response) {
    (void)context;

    auto profile = service_->GetProfile(request->user_id());
    if (!profile) {
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "user not found");
    }

    response->set_user_id(profile->user_id);
    response->set_username(profile->username);
    response->set_nickname(profile->nickname);
    response->set_avatar_url(profile->avatar_url);
    response->set_signature(profile->signature);
    response->set_gender(profile->gender);
    response->set_created_at(profile->created_at);
    return ::grpc::Status::OK;
}

::grpc::Status AuthHandler::UpdateProfile(::grpc::ServerContext* context,
                                           const ::swift::auth::UpdateProfileRequest* request,
                                           ::swift::common::CommonResponse* response) {
    (void)context;

    auto result = service_->UpdateProfile(
        request->user_id(),
        request->nickname(),
        request->avatar_url(),
        request->signature());

    if (result.success) {
        response->set_code(static_cast<int>(swift::ErrorCode::OK));
    } else {
        response->set_code(swift::ErrorCodeToInt(result.error_code));
        response->set_message(result.error);
    }
    return ::grpc::Status::OK;
}

}  // namespace swift::auth
