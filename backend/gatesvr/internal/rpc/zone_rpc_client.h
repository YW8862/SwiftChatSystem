#pragma once

#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "zone.grpc.pb.h"

namespace swift::gate {

/** HandleClientRequest 返回体 */
struct HandleClientRequestResult {
    int code = 0;
    std::string message;
    std::string payload;
    std::string request_id;
};

/**
 * ZoneSvr gRPC 客户端
 * 每次调用在 ClientContext 上 AddMetadata("x-internal-secret", zonesvr_internal_secret)
 */
class ZoneRpcClient {
public:
    ZoneRpcClient();
    ~ZoneRpcClient();

    void Init(const std::string& zone_svr_addr,
              const std::string& zonesvr_internal_secret);

    /** 用户上线 */
    bool UserOnline(const std::string& user_id, const std::string& gate_id,
                    const std::string& device_type, const std::string& device_id);

    /** 用户下线 */
    bool UserOffline(const std::string& user_id, const std::string& gate_id);

    /** Gate 注册（启动时调用） */
    bool GateRegister(const std::string& gate_id, const std::string& address,
                     int current_connections);

    /** Gate 心跳（建议每 30s 调用，Zone 更新 gate 存活与连接数） */
    bool GateHeartbeat(const std::string& gate_id, int current_connections);

    /** 客户端业务请求统一入口：转发到 Zone，返回 (code, message, payload, request_id)；token 为已登录连接的 JWT，供 Zone 调业务服务时注入 */
    bool HandleClientRequest(const std::string& conn_id, const std::string& user_id,
                            const std::string& cmd, const std::string& payload,
                            const std::string& request_id, const std::string& token,
                            HandleClientRequestResult* result);

private:
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<swift::zone::ZoneService::Stub> stub_;
    std::string zonesvr_internal_secret_;
};

}  // namespace swift::gate
