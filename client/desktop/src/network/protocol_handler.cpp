#include "protocol_handler.h"
#include "gate.pb.h"

#include <QDebug>

ProtocolHandler::ProtocolHandler(QObject *parent) : QObject(parent) {
}

ProtocolHandler::~ProtocolHandler() = default;

void ProtocolHandler::sendRequest(const QString& cmd, const QByteArray& payload,
                                   ResponseCallback callback) {
    QString requestId = generateRequestId();

    if (callback) {
        m_pendingRequests[requestId] = callback;
    }

    swift::gate::ClientMessage msg;
    msg.set_cmd(cmd.toStdString());
    if (!payload.isEmpty()) {
        msg.set_payload(payload.data(), static_cast<int>(payload.size()));
    }
    msg.set_request_id(requestId.toStdString());

    std::string serialized;
    if (!msg.SerializeToString(&serialized)) {
        qWarning() << "ProtocolHandler: failed to serialize ClientMessage";
        if (callback) {
            callback(-1, QByteArray());
            m_pendingRequests.erase(requestId);
        }
        return;
    }

    emit dataToSend(QByteArray(serialized.data(), static_cast<int>(serialized.size())));
}

void ProtocolHandler::handleMessage(const QByteArray& data) {
    swift::gate::ServerMessage msg;
    if (!msg.ParseFromArray(data.data(), data.size())) {
        qWarning() << "ProtocolHandler: failed to parse ServerMessage";
        return;
    }

    QString cmd = QString::fromStdString(msg.cmd());
    QString requestId = QString::fromStdString(msg.request_id());
    int code = msg.code();
    QByteArray payload;
    if (msg.payload().size() > 0) {
        payload = QByteArray(msg.payload().data(), static_cast<int>(msg.payload().size()));
    }

    // 请求响应：request_id 非空且在 m_pendingRequests 中存在
    if (!requestId.isEmpty() && m_pendingRequests.count(requestId)) {
        auto it = m_pendingRequests.find(requestId);
        auto callback = it->second;
        m_pendingRequests.erase(it);
        if (callback) {
            callback(code, payload);
        }
        return;
    }

    // 推送通知：按 cmd 分发
    if (cmd == "chat.new_message") {
        emit newMessageNotify(payload);
    } else if (cmd == "chat.recall") {
        emit recallNotify(payload);
    } else if (cmd == "chat.read_receipt") {
        emit readReceiptNotify(payload);
    } else if (cmd == "friend.request") {
        emit friendRequestNotify(payload);
    } else if (cmd == "friend.accepted") {
        emit friendAcceptedNotify(payload);
    } else if (cmd == "user.status_change") {
        emit userStatusChangeNotify(payload);
    } else if (cmd == "system.kicked") {
        QString reason;
        if (!payload.isEmpty()) {
            swift::gate::KickedNotify kicked;
            if (kicked.ParseFromArray(payload.data(), payload.size())) {
                reason = QString::fromStdString(kicked.reason());
            }
        }
        emit kickedNotify(reason);
    } else {
        qDebug() << "ProtocolHandler: unhandled push cmd=" << cmd;
    }
}

QString ProtocolHandler::generateRequestId() {
    return QString("req_%1").arg(++m_requestIdCounter);
}

void ProtocolHandler::clearPendingRequests() {
    const int NETWORK_ERROR_CODE = -1;
    auto pending = std::move(m_pendingRequests);
    m_pendingRequests.clear();
    for (auto& pair : pending) {
        if (pair.second) {
            pair.second(NETWORK_ERROR_CODE, QByteArray());
        }
    }
}
