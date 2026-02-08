#pragma once

#include <grpcpp/grpcpp.h>
#include <memory>
#include "online.grpc.pb.h"

namespace swift::online {

class OnlineServiceCore;

/**
 * 对外 gRPC 层（Handler）
 * 实现 proto 定义的 OnlineService：Login、Logout、ValidateToken。
 */
class OnlineHandler : public OnlineService::Service {
public:
    explicit OnlineHandler(std::shared_ptr<OnlineServiceCore> service);
    ~OnlineHandler() override;

    ::grpc::Status Login(::grpc::ServerContext* context,
                         const ::swift::online::LoginRequest* request,
                         ::swift::online::LoginResponse* response) override;

    ::grpc::Status Logout(::grpc::ServerContext* context,
                          const ::swift::online::LogoutRequest* request,
                          ::swift::common::CommonResponse* response) override;

    ::grpc::Status ValidateToken(::grpc::ServerContext* context,
                                 const ::swift::online::TokenRequest* request,
                                 ::swift::online::TokenResponse* response) override;

private:
    std::shared_ptr<OnlineServiceCore> service_;
};

}  // namespace swift::online
