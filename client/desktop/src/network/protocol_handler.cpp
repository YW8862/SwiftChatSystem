#include "protocol_handler.h"

ProtocolHandler::ProtocolHandler(QObject *parent) : QObject(parent) {
}

ProtocolHandler::~ProtocolHandler() = default;

void ProtocolHandler::sendRequest(const QString& cmd, const QByteArray& payload,
                                   ResponseCallback callback) {
    QString requestId = generateRequestId();
    
    if (callback) {
        m_pendingRequests[requestId] = callback;
    }
    
    // TODO: 构造 ClientMessage 并序列化
    // swift::gate::ClientMessage msg;
    // msg.set_cmd(cmd.toStdString());
    // msg.set_payload(payload.data(), payload.size());
    // msg.set_request_id(requestId.toStdString());
    
    // QByteArray data;
    // msg.SerializeToArray(...);
    // emit dataToSend(data);
}

void ProtocolHandler::handleMessage(const QByteArray& data) {
    // TODO: 解析 ServerMessage
    // swift::gate::ServerMessage msg;
    // msg.ParseFromArray(data.data(), data.size());
    
    // QString cmd = QString::fromStdString(msg.cmd());
    // QString requestId = QString::fromStdString(msg.request_id());
    
    // 如果是响应
    // if (!requestId.isEmpty() && m_pendingRequests.count(requestId)) {
    //     auto callback = m_pendingRequests[requestId];
    //     m_pendingRequests.erase(requestId);
    //     callback(msg.code(), QByteArray(msg.payload().data(), msg.payload().size()));
    //     return;
    // }
    
    // 如果是推送通知
    // if (cmd == "chat.new_message") emit newMessageNotify(...);
    // else if (cmd == "chat.recall") emit recallNotify(...);
    // ...
}

QString ProtocolHandler::generateRequestId() {
    return QString("req_%1").arg(++m_requestIdCounter);
}
