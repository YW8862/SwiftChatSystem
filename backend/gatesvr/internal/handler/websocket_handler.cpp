#include "websocket_handler.h"

namespace swift::gate {

WebSocketHandler::WebSocketHandler(std::shared_ptr<GateService> service)
    : service_(std::move(service)) {}

WebSocketHandler::~WebSocketHandler() = default;

void WebSocketHandler::OnConnect(const std::string& conn_id) {
    // TODO
}

void WebSocketHandler::OnMessage(const std::string& conn_id, const std::string& data) {
    // TODO: 解析 ClientMessage，转发到 ZoneSvr
}

void WebSocketHandler::OnDisconnect(const std::string& conn_id) {
    // TODO
}

bool WebSocketHandler::SendToClient(const std::string& conn_id, const std::string& data) {
    return false;
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
