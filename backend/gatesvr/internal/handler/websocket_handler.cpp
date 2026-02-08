#include "websocket_handler.h"
#include "service/gate_service.h"
#include "gate.pb.h"

namespace swift::gate {

WebSocketHandler::WebSocketHandler(std::shared_ptr<GateService> service)
    : service_(std::move(service)) {}

WebSocketHandler::~WebSocketHandler() = default;

void WebSocketHandler::OnConnect(const std::string& conn_id) {
    (void)conn_id;
}

void WebSocketHandler::OnMessage(const std::string& conn_id, const std::string& data) {
    swift::gate::ClientMessage msg;
    if (!msg.ParseFromString(data)) {
        return;  // 解析失败，忽略
    }
    std::string cmd = msg.cmd();
    std::string payload = msg.payload();
    std::string request_id = msg.request_id();
    service_->HandleClientMessage(conn_id, cmd, payload, request_id);
}

void WebSocketHandler::OnDisconnect(const std::string& conn_id) {
    (void)conn_id;
}

bool WebSocketHandler::SendToClient(const std::string& conn_id, const std::string& data) {
    return service_->SendToConn(conn_id, data);
}

GateInternalHandler::GateInternalHandler(std::shared_ptr<GateService> service)
    : service_(std::move(service)) {}

GateInternalHandler::~GateInternalHandler() = default;

bool GateInternalHandler::PushMessage(const std::string& user_id,
                                       const std::string& cmd,
                                       const std::string& payload) {
    return false;
}

bool GateInternalHandler::DisconnectUser(const std::string& user_id,
                                          const std::string& reason) {
    return false;
}

}  // namespace swift::gate
