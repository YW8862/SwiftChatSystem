/**
 * @file zone_rpc_client.cpp
 * @brief ZoneSvr gRPC 客户端：UserOnline、UserOffline、GateRegister、HandleClientRequest
 * 每次调用在 ClientContext 上 AddMetadata("x-internal-secret", zonesvr_internal_secret)
 */

#include "zone_rpc_client.h"
#include "zone.pb.h"
#include <chrono>

namespace swift::gate {

static const char kMetadataKey[] = "x-internal-secret";

ZoneRpcClient::ZoneRpcClient() = default;

ZoneRpcClient::~ZoneRpcClient() = default;

void ZoneRpcClient::Init(const std::string& zone_svr_addr,
                         const std::string& zonesvr_internal_secret) {
    zonesvr_internal_secret_ = zonesvr_internal_secret;
    grpc::ChannelArguments args;
    args.SetMaxReceiveMessageSize(64 * 1024 * 1024);
    args.SetMaxSendMessageSize(64 * 1024 * 1024);
    channel_ = grpc::CreateCustomChannel(
        zone_svr_addr,
        grpc::InsecureChannelCredentials(),
        args);
    stub_ = swift::zone::ZoneService::NewStub(channel_);
}

static void AddInternalSecret(grpc::ClientContext* ctx,
                             const std::string& secret) {
    if (!secret.empty())
        ctx->AddMetadata(kMetadataKey, secret);
}

bool ZoneRpcClient::UserOnline(const std::string& user_id,
                               const std::string& gate_id,
                               const std::string& device_type,
                               const std::string& device_id) {
    if (!stub_) return false;
    swift::zone::UserOnlineRequest req;
    req.set_user_id(user_id);
    req.set_gate_id(gate_id);
    req.set_device_type(device_type);
    req.set_device_id(device_id);
    swift::common::CommonResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
    AddInternalSecret(&ctx, zonesvr_internal_secret_);
    grpc::Status status = stub_->UserOnline(&ctx, req, &resp);
    return status.ok() && resp.code() == 0;
}

bool ZoneRpcClient::UserOffline(const std::string& user_id,
                                const std::string& gate_id) {
    if (!stub_) return false;
    swift::zone::UserOfflineRequest req;
    req.set_user_id(user_id);
    req.set_gate_id(gate_id);
    swift::common::CommonResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
    AddInternalSecret(&ctx, zonesvr_internal_secret_);
    grpc::Status status = stub_->UserOffline(&ctx, req, &resp);
    return status.ok() && resp.code() == 0;
}

bool ZoneRpcClient::GateRegister(const std::string& gate_id,
                                const std::string& address,
                                int current_connections) {
    if (!stub_) return false;
    swift::zone::GateRegisterRequest req;
    req.set_gate_id(gate_id);
    req.set_address(address);
    req.set_current_connections(current_connections);
    swift::common::CommonResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
    AddInternalSecret(&ctx, zonesvr_internal_secret_);
    grpc::Status status = stub_->GateRegister(&ctx, req, &resp);
    return status.ok() && resp.code() == 0;
}

bool ZoneRpcClient::GateHeartbeat(const std::string& gate_id,
                                  int current_connections) {
    if (!stub_) return false;
    swift::zone::GateHeartbeatRequest req;
    req.set_gate_id(gate_id);
    req.set_current_connections(current_connections);
    swift::common::CommonResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
    AddInternalSecret(&ctx, zonesvr_internal_secret_);
    grpc::Status status = stub_->GateHeartbeat(&ctx, req, &resp);
    return status.ok() && resp.code() == 0;
}

bool ZoneRpcClient::HandleClientRequest(const std::string& conn_id,
                                        const std::string& user_id,
                                        const std::string& cmd,
                                        const std::string& payload,
                                        const std::string& request_id,
                                        const std::string& token,
                                        HandleClientRequestResult* result) {
    if (!stub_ || !result) return false;
    swift::zone::HandleClientRequestRequest req;
    req.set_conn_id(conn_id);
    req.set_user_id(user_id);
    req.set_cmd(cmd);
    req.set_payload(payload);
    req.set_request_id(request_id);
    if (!token.empty()) req.set_token(token);
    swift::zone::HandleClientRequestResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(10));
    AddInternalSecret(&ctx, zonesvr_internal_secret_);
    grpc::Status status = stub_->HandleClientRequest(&ctx, req, &resp);
    if (!status.ok()) {
        result->code = -1;
        result->message = status.error_message();
        result->request_id = request_id;
        return false;
    }
    result->code = resp.code();
    result->message = resp.message();
    result->payload = resp.payload();
    result->request_id = resp.request_id();
    return true;
}

}  // namespace swift::gate
