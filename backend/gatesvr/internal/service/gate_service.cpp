#include "gate_service.h"
#include "config/config.h"
#include "gate.pb.h"
#include "rpc/zone_rpc_client.h"
#include "swift/error_code.h"
#include "zone.pb.h"
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <unistd.h>

namespace swift::gate {

namespace {

/** 未配置 gate_id 时用 hostname:grpc_port，保证多实例唯一 */
std::string FallbackGateId(int grpc_port) {
    char name[256];
    if (gethostname(name, sizeof(name)) != 0)
        name[0] = '\0';
    return std::string(name) + ":" + std::to_string(grpc_port);
}

}  // namespace

GateService::GateService() {
    gate_id_.clear();
}

GateService::~GateService() = default;

void GateService::Init(const GateConfig& config) {
    gate_id_ = config.gate_id.empty() ? FallbackGateId(config.grpc_port) : config.gate_id;
    zone_svr_addr_ = config.zone_svr_addr;
    zonesvr_internal_secret_ = config.zonesvr_internal_secret;
    heartbeat_timeout_seconds_ = config.heartbeat_timeout_seconds > 0
        ? config.heartbeat_timeout_seconds : 90;
    zone_client_ = std::make_unique<ZoneRpcClient>();
    zone_client_->Init(zone_svr_addr_, zonesvr_internal_secret_);
}

void GateService::AddConnection(const std::string& conn_id) {
    std::unique_lock lock(mutex_);
    Connection c;
    c.conn_id = conn_id;
    c.connected_at = c.last_heartbeat =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    connections_[conn_id] = std::move(c);
}

void GateService::RemoveConnection(const std::string& conn_id) {
    std::string user_id;
    {
        std::unique_lock lock(mutex_);
        auto it = connections_.find(conn_id);
        if (it != connections_.end()) {
            user_id = it->second.user_id;
            if (!user_id.empty())
                user_to_conn_.erase(user_id);
            connections_.erase(it);
        }
    }
    if (!user_id.empty())
        NotifyUserOffline(user_id);
}

Connection* GateService::GetConnection(const std::string& conn_id) {
    std::shared_lock lock(mutex_);
    auto it = connections_.find(conn_id);
    return it != connections_.end() ? &it->second : nullptr;
}

bool GateService::BindUser(const std::string& conn_id, const std::string& user_id,
                           const std::string& token,
                           const std::string& device_id, const std::string& device_type) {
    std::unique_lock lock(mutex_);
    auto it = connections_.find(conn_id);
    if (it == connections_.end()) return false;
    if (!it->second.user_id.empty())
        user_to_conn_.erase(it->second.user_id);
    it->second.user_id = user_id;
    it->second.token = token;
    it->second.device_id = device_id;
    it->second.device_type = device_type;
    it->second.authenticated = true;
    user_to_conn_[user_id] = conn_id;
    return true;
}

void GateService::UnbindUser(const std::string& user_id) {
    std::unique_lock lock(mutex_);
    user_to_conn_.erase(user_id);
    for (auto& [cid, c] : connections_) {
        if (c.user_id == user_id) {
            c.user_id.clear();
            c.device_id.clear();
            c.device_type.clear();
            c.authenticated = false;
            break;
        }
    }
}

std::string GateService::GetConnIdByUser(const std::string& user_id) {
    std::shared_lock lock(mutex_);
    auto it = user_to_conn_.find(user_id);
    return it != user_to_conn_.end() ? it->second : "";
}

Connection* GateService::GetConnectionByUserId(const std::string& user_id) {
    std::shared_lock lock(mutex_);
    auto it = user_to_conn_.find(user_id);
    if (it == user_to_conn_.end()) return nullptr;
    auto cIt = connections_.find(it->second);
    return cIt != connections_.end() ? &cIt->second : nullptr;
}

void GateService::HandleClientMessage(const std::string& conn_id, const std::string& cmd,
                                       const std::string& payload, const std::string& request_id) {
    if (cmd == "auth.login") {
        HandleLogin(conn_id, payload, request_id);
    } else if (cmd == "heartbeat") {
        HandleHeartbeat(conn_id, request_id);
    } else if ((cmd.size() >= 5 && cmd.substr(0, 5) == "chat.") ||
               (cmd.size() >= 7 && cmd.substr(0, 7) == "friend.") ||
               (cmd.size() >= 6 && cmd.substr(0, 6) == "group.") ||
               (cmd.size() >= 5 && cmd.substr(0, 5) == "file.")) {
        ForwardToZone(conn_id, cmd, payload, request_id);
    } else {
        SendResponse(conn_id, cmd, request_id,
                     swift::ErrorCodeToInt(swift::ErrorCode::UNSUPPORTED),
                     swift::ErrorCodeToString(swift::ErrorCode::UNSUPPORTED));
    }
}

bool GateService::PushToUser(const std::string& user_id, const std::string& cmd,
                              const std::string& payload) {
    std::string conn_id = GetConnIdByUser(user_id);
    if (conn_id.empty()) return false;
    // 组 ServerMessage 并发送
    swift::gate::ServerMessage msg;
    msg.set_cmd(cmd.empty() ? "message" : cmd);
    msg.set_payload(payload);
    msg.set_code(0);
    std::string data;
    if (!msg.SerializeToString(&data)) return false;
    return SendToConn(conn_id, data);
}

void GateService::CheckHeartbeat() {
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    int64_t timeout_ms = static_cast<int64_t>(heartbeat_timeout_seconds_) * 1000;
    std::vector<std::string> to_close;
    {
        std::shared_lock lock(mutex_);
        for (const auto& [conn_id, c] : connections_) {
            if (now_ms - c.last_heartbeat > timeout_ms)
                to_close.push_back(conn_id);
        }
    }
    for (const std::string& conn_id : to_close)
        CloseConnection(conn_id);
}

void GateService::SetCloseCallback(const std::string& conn_id, CloseCallback fn) {
    std::unique_lock lock(mutex_);
    close_callbacks_[conn_id] = std::move(fn);
}

void GateService::RemoveCloseCallback(const std::string& conn_id) {
    std::unique_lock lock(mutex_);
    close_callbacks_.erase(conn_id);
}

void GateService::CloseConnection(const std::string& conn_id) {
    CloseCallback fn;
    {
        std::shared_lock lock(mutex_);
        auto it = close_callbacks_.find(conn_id);
        if (it == close_callbacks_.end()) return;
        fn = it->second;
    }
    if (fn) fn();
}

int GateService::GetConnectionCount() const {
    std::shared_lock lock(mutex_);
    return static_cast<int>(connections_.size());
}

void GateService::SetSendCallback(const std::string& conn_id, SendCallback fn) {
    std::unique_lock lock(mutex_);
    send_callbacks_[conn_id] = std::move(fn);
}

void GateService::RemoveSendCallback(const std::string& conn_id) {
    std::unique_lock lock(mutex_);
    send_callbacks_.erase(conn_id);
}

bool GateService::SendToConn(const std::string& conn_id, const std::string& data) {
    SendCallback fn;
    {
        std::shared_lock lock(mutex_);
        auto it = send_callbacks_.find(conn_id);
        if (it == send_callbacks_.end()) return false;
        fn = it->second;
    }
    return fn && fn(data);
}

void GateService::NotifyUserOffline(const std::string& user_id) {
    if (!zone_client_) return;
    zone_client_->UserOffline(user_id, gate_id_);
}

bool GateService::SendResponse(const std::string& conn_id, const std::string& cmd,
                                const std::string& request_id, int code,
                                const std::string& message, const std::string& payload) {
    swift::gate::ServerMessage msg;
    msg.set_cmd(cmd);
    msg.set_request_id(request_id);
    msg.set_code(code);
    msg.set_message(message);
    if (!payload.empty())
        msg.set_payload(payload);
    std::string data;
    if (!msg.SerializeToString(&data)) return false;
    return SendToConn(conn_id, data);
}

void GateService::UpdateHeartbeat(const std::string& conn_id) {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::unique_lock lock(mutex_);
    auto it = connections_.find(conn_id);
    if (it != connections_.end())
        it->second.last_heartbeat = now;
}

void GateService::HandleLogin(const std::string& conn_id, const std::string& payload,
                              const std::string& request_id) {
    swift::gate::ClientLoginRequest req;
    if (!req.ParseFromString(payload)) {
        SendResponse(conn_id, "auth.login", request_id,
                     swift::ErrorCodeToInt(swift::ErrorCode::INVALID_PARAM),
                     swift::ErrorCodeToString(swift::ErrorCode::INVALID_PARAM));
        return;
    }
    if (!zone_client_) {
        SendResponse(conn_id, "auth.login", request_id,
                     swift::ErrorCodeToInt(swift::ErrorCode::UPSTREAM_UNAVAILABLE),
                     swift::ErrorCodeToString(swift::ErrorCode::UPSTREAM_UNAVAILABLE));
        return;
    }
    // 通过 ZoneSvr 转发到 OnlineSvr.ValidateToken
    swift::zone::AuthValidateTokenPayload validate_req;
    validate_req.set_token(req.token());
    std::string validate_payload;
    if (!validate_req.SerializeToString(&validate_payload)) {
        SendResponse(conn_id, "auth.login", request_id,
                     swift::ErrorCodeToInt(swift::ErrorCode::INVALID_PARAM),
                     swift::ErrorCodeToString(swift::ErrorCode::INVALID_PARAM));
        return;
    }
    HandleClientRequestResult result;
    if (!zone_client_->HandleClientRequest(conn_id, "", "auth.validate_token",
                                           validate_payload, request_id, "", &result)) {
        int code = result.code < 0 ? swift::ErrorCodeToInt(swift::ErrorCode::RPC_FAILED) : result.code;
        std::string msg = result.message.empty() ? swift::ErrorCodeToString(swift::ErrorCode::RPC_FAILED) : result.message;
        SendResponse(conn_id, "auth.login", request_id, code, msg);
        return;
    }
    if (result.code != 0) {
        SendResponse(conn_id, "auth.login", request_id, result.code, result.message);
        return;
    }
    swift::zone::AuthValidateTokenResponsePayload validate_resp;
    if (!validate_resp.ParseFromString(result.payload) || validate_resp.user_id().empty()) {
        SendResponse(conn_id, "auth.login", request_id,
                     swift::ErrorCodeToInt(swift::ErrorCode::TOKEN_INVALID),
                     swift::ErrorCodeToString(swift::ErrorCode::TOKEN_INVALID));
        return;
    }
    std::string user_id = validate_resp.user_id();
    if (!BindUser(conn_id, user_id, req.token(), req.device_id(), req.device_type())) {
        SendResponse(conn_id, "auth.login", request_id,
                     swift::ErrorCodeToInt(swift::ErrorCode::INTERNAL_ERROR),
                     swift::ErrorCodeToString(swift::ErrorCode::INTERNAL_ERROR));
        return;
    }
    zone_client_->UserOnline(user_id, gate_id_, req.device_type(), req.device_id());
    SendResponse(conn_id, "auth.login", request_id,
                 swift::ErrorCodeToInt(swift::ErrorCode::OK),
                 swift::ErrorCodeToString(swift::ErrorCode::OK));
}

void GateService::HandleHeartbeat(const std::string& conn_id,
                                  const std::string& request_id) {
    UpdateHeartbeat(conn_id);
    swift::gate::HeartbeatResponse resp;
    resp.set_server_time(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    std::string payload;
    if (!resp.SerializeToString(&payload))
        return;
    SendResponse(conn_id, "heartbeat", request_id,
                 swift::ErrorCodeToInt(swift::ErrorCode::OK),
                 swift::ErrorCodeToString(swift::ErrorCode::OK), payload);
}

void GateService::ForwardToZone(const std::string& conn_id, const std::string& cmd,
                                const std::string& payload, const std::string& request_id) {
    if (!zone_client_) {
        SendResponse(conn_id, cmd, request_id,
                     swift::ErrorCodeToInt(swift::ErrorCode::UPSTREAM_UNAVAILABLE),
                     swift::ErrorCodeToString(swift::ErrorCode::UPSTREAM_UNAVAILABLE));
        return;
    }
    std::string user_id;
    std::string token;
    {
        Connection* c = GetConnection(conn_id);
        if (c) {
            user_id = c->user_id;
            token = c->token;
        }
    }
    HandleClientRequestResult result;
    if (!zone_client_->HandleClientRequest(conn_id, user_id, cmd, payload, request_id, token, &result)) {
        int code = result.code < 0 ? swift::ErrorCodeToInt(swift::ErrorCode::RPC_FAILED) : result.code;
        std::string msg = result.message.empty() ? swift::ErrorCodeToString(swift::ErrorCode::RPC_FAILED) : result.message;
        SendResponse(conn_id, cmd, request_id, code, msg);
        return;
    }
    SendResponse(conn_id, cmd, result.request_id, result.code, result.message, result.payload);
}

bool GateService::RegisterGate(const std::string& grpc_address) {
    if (!zone_client_) return false;
    return zone_client_->GateRegister(gate_id_, grpc_address, GetConnectionCount());
}

bool GateService::GateHeartbeat() {
    if (!zone_client_) return false;
    return zone_client_->GateHeartbeat(gate_id_, GetConnectionCount());
}

}  // namespace swift::gate
