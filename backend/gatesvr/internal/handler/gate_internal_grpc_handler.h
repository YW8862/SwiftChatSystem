#pragma once

#include "gate.grpc.pb.h"
#include <memory>

namespace swift::gate {

class GateService;

/**
 * GateInternalService gRPC 实现：ZoneSvr 调用 PushMessage/DisconnectUser。
 */
class GateInternalGrpcHandler : public swift::gate::GateInternalService::Service {
public:
    explicit GateInternalGrpcHandler(std::shared_ptr<GateService> service);
    ~GateInternalGrpcHandler() override;

    ::grpc::Status PushMessage(::grpc::ServerContext* context,
                               const ::swift::gate::PushMessageRequest* request,
                               ::swift::common::CommonResponse* response) override;

    ::grpc::Status DisconnectUser(::grpc::ServerContext* context,
                                  const ::swift::gate::DisconnectUserRequest* request,
                                  ::swift::common::CommonResponse* response) override;

private:
    std::shared_ptr<GateService> service_;
};

}  // namespace swift::gate
