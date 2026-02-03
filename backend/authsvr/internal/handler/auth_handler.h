#pragma once

#include <memory>
#include "auth.grpc.pb.h"

namespace swift::auth {

class AuthServiceCore;  // 业务逻辑类，与 proto 生成的 AuthService 区分

/**
 * 对外 API 层（Handler）
 * 直接实现 proto 定义的 gRPC AuthService 接口：Register、VerifyCredentials、GetProfile、UpdateProfile。
 */
class AuthHandler : public AuthService::Service {
public:
    explicit AuthHandler(std::shared_ptr<AuthServiceCore> service);
    ~AuthHandler() override;

    ::grpc::Status Register(::grpc::ServerContext* context,
                            const ::swift::auth::RegisterRequest* request,
                            ::swift::auth::RegisterResponse* response) override;

    ::grpc::Status VerifyCredentials(::grpc::ServerContext* context,
                                      const ::swift::auth::VerifyCredentialsRequest* request,
                                      ::swift::auth::VerifyCredentialsResponse* response) override;

    ::grpc::Status GetProfile(::grpc::ServerContext* context,
                              const ::swift::auth::GetProfileRequest* request,
                              ::swift::auth::UserProfile* response) override;

    ::grpc::Status UpdateProfile(::grpc::ServerContext* context,
                                  const ::swift::auth::UpdateProfileRequest* request,
                                  ::swift::common::CommonResponse* response) override;

private:
    std::shared_ptr<AuthServiceCore> service_;
};

}  // namespace swift::auth
