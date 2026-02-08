/**
 * @file zone_handler.h
 * @brief ZoneService gRPC 服务实现
 *
 * 继承 proto 生成的 ZoneService::Service，将每个 RPC 委托给 ZoneService 业务类。
 */

#pragma once

#include "zone.grpc.pb.h"
#include <memory>

namespace swift::zone {

class ZoneServiceImpl;

/**
 * ZoneHandler 实现 proto 定义的 ZoneService，每个 RPC 解析 request、
 * 调用 ZoneServiceImpl 对应方法、将结果写入 response、返回 Status::OK。
 */
class ZoneHandler : public swift::zone::ZoneService::Service {
public:
    explicit ZoneHandler(std::shared_ptr<ZoneServiceImpl> service);
    ~ZoneHandler() override;

    ::grpc::Status UserOnline(::grpc::ServerContext* context,
                              const ::swift::zone::UserOnlineRequest* request,
                              ::swift::common::CommonResponse* response) override;

    ::grpc::Status UserOffline(::grpc::ServerContext* context,
                               const ::swift::zone::UserOfflineRequest* request,
                               ::swift::common::CommonResponse* response) override;

    ::grpc::Status RouteMessage(::grpc::ServerContext* context,
                                const ::swift::zone::RouteMessageRequest* request,
                                ::swift::zone::RouteMessageResponse* response) override;

    ::grpc::Status Broadcast(::grpc::ServerContext* context,
                            const ::swift::zone::BroadcastRequest* request,
                            ::swift::zone::BroadcastResponse* response) override;

    ::grpc::Status GetUserStatus(::grpc::ServerContext* context,
                                const ::swift::zone::GetUserStatusRequest* request,
                                ::swift::zone::GetUserStatusResponse* response) override;

    ::grpc::Status PushToUser(::grpc::ServerContext* context,
                             const ::swift::zone::PushToUserRequest* request,
                             ::swift::common::CommonResponse* response) override;

    ::grpc::Status KickUser(::grpc::ServerContext* context,
                            const ::swift::zone::KickUserRequest* request,
                            ::swift::common::CommonResponse* response) override;

    ::grpc::Status GateRegister(::grpc::ServerContext* context,
                                const ::swift::zone::GateRegisterRequest* request,
                                ::swift::common::CommonResponse* response) override;

    ::grpc::Status GateHeartbeat(::grpc::ServerContext* context,
                                 const ::swift::zone::GateHeartbeatRequest* request,
                                 ::swift::common::CommonResponse* response) override;

    /// 客户端业务请求统一入口：按 cmd 分发到各 System，各 System 通过 gRPC 调后端
    ::grpc::Status HandleClientRequest(::grpc::ServerContext* context,
                                       const ::swift::zone::HandleClientRequestRequest* request,
                                       ::swift::zone::HandleClientRequestResponse* response) override;

private:
    std::shared_ptr<ZoneServiceImpl> service_;
};

}  // namespace swift::zone
