#include "protocol_handler.h"
#include "gate.pb.h"
#include "swift/log_helper.h"

#include <QDateTime>

ProtocolHandler::ProtocolHandler(QObject *parent) : QObject(parent) {
}

ProtocolHandler::~ProtocolHandler() = default;

void ProtocolHandler::sendRequest(const QString& cmd, const QByteArray& payload,
                                  ResponseCallback callback) {
    sendRequestImpl(cmd, payload, callback, nullptr);
}

void ProtocolHandler::sendRequest(const QString& cmd, const QByteArray& payload,
                                  ResponseCallbackWithMessage callback) {
    sendRequestImpl(cmd, payload, nullptr, callback);
}

void ProtocolHandler::sendRequestImpl(const QString& cmd, const QByteArray& payload,
                                      ResponseCallback callback,
                                      ResponseCallbackWithMessage callbackWithMessage) {
    QString requestId = generateRequestId();
    const qint64 startMs = QDateTime::currentMSecsSinceEpoch();

    swift::gate::ClientMessage msg;
    msg.set_cmd(cmd.toStdString());
    if (!payload.isEmpty()) {
        msg.set_payload(payload.data(), static_cast<int>(payload.size()));
    }
    msg.set_request_id(requestId.toStdString());

    std::string serialized;
    if (!msg.SerializeToString(&serialized)) {
        LogError("[ProtocolHandler] failed to serialize ClientMessage, cmd=" << cmd.toStdString());
        if (callbackWithMessage) {
            callbackWithMessage(-1, QByteArray(), QString());
        } else if (callback) {
            callback(-1, QByteArray());
        }
        return;
    }

    LogDebug("[ProtocolHandler] request sent, cmd=" << cmd.toStdString()
             << ", request_id=" << requestId.toStdString()
             << ", payload_size=" << payload.size());

    if (callback || callbackWithMessage) {
        auto* timeoutTimer = new QTimer(this);
        timeoutTimer->setSingleShot(true);
        QObject::connect(timeoutTimer, &QTimer::timeout, this, [this, requestId]() {
            auto it = m_pendingRequests.find(requestId);
            if (it == m_pendingRequests.end()) {
                return;
            }
            auto pending = std::move(it->second);
            m_pendingRequests.erase(it);
            if (pending.timeout_timer) {
                pending.timeout_timer->deleteLater();
            }
            if (pending.callback_with_message) {
                const int REQUEST_TIMEOUT_CODE = -2;
                pending.callback_with_message(REQUEST_TIMEOUT_CODE, QByteArray(), QString());
            } else if (pending.callback) {
                const int REQUEST_TIMEOUT_CODE = -2;
                pending.callback(REQUEST_TIMEOUT_CODE, QByteArray());
            }
            const qint64 costMs = pending.start_ms > 0
                ? (QDateTime::currentMSecsSinceEpoch() - pending.start_ms)
                : -1;
            LogWarning("[ProtocolHandler] request timeout, cmd=" << pending.cmd.toStdString()
                       << ", request_id=" << requestId.toStdString()
                       << ", cost_ms=" << costMs);
        });
        timeoutTimer->start(m_requestTimeoutMs);

        PendingRequest pending;
        pending.callback = callback;
        pending.callback_with_message = callbackWithMessage;
        pending.timeout_timer = timeoutTimer;
        pending.cmd = cmd;
        pending.start_ms = startMs;
        m_pendingRequests[requestId] = std::move(pending);
    }

    emit dataToSend(QByteArray(serialized.data(), static_cast<int>(serialized.size())));
}

void ProtocolHandler::handleMessage(const QByteArray& data) {
    swift::gate::ServerMessage msg;
    if (!msg.ParseFromArray(data.data(), data.size())) {
        LogWarning("[ProtocolHandler] failed to parse ServerMessage, bytes=" << data.size());
        return;
    }

    QString cmd = QString::fromStdString(msg.cmd());
    QString requestId = QString::fromStdString(msg.request_id());
    int code = msg.code();
    QString serverMessage = QString::fromStdString(msg.message());
    QByteArray payload;
    if (msg.payload().size() > 0) {
        payload = QByteArray(msg.payload().data(), static_cast<int>(msg.payload().size()));
    }

    // 请求响应：request_id 非空且在 m_pendingRequests 中存在
    if (!requestId.isEmpty() && m_pendingRequests.count(requestId)) {
        auto it = m_pendingRequests.find(requestId);
        auto pending = std::move(it->second);
        m_pendingRequests.erase(it);
        const qint64 costMs = pending.start_ms > 0
            ? (QDateTime::currentMSecsSinceEpoch() - pending.start_ms)
            : -1;
        if (pending.timeout_timer) {
            pending.timeout_timer->stop();
            pending.timeout_timer->deleteLater();
        }
        LogDebug("[ProtocolHandler] request done, cmd=" << pending.cmd.toStdString()
                 << ", request_id=" << requestId.toStdString()
                 << ", code=" << code
                 << ", cost_ms=" << costMs
                 << ", payload_size=" << payload.size());
        if (pending.callback_with_message) {
            pending.callback_with_message(code, payload, serverMessage);
        } else if (pending.callback) {
            pending.callback(code, payload);
        }
        return;
    }

    // 推送通知：按 cmd 分发
    if (cmd == "chat.new_message" || cmd == "chat.message") {
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
        LogDebug("[ProtocolHandler] unhandled push cmd=" << cmd.toStdString());
    }
}

QString ProtocolHandler::generateRequestId() {
    return QString("req_%1").arg(++m_requestIdCounter);
}

void ProtocolHandler::clearPendingRequests() {
    const int NETWORK_ERROR_CODE = -1;
    auto pending = std::move(m_pendingRequests);
    m_pendingRequests.clear();
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    for (auto& pair : pending) {
        auto& request = pair.second;
        const qint64 costMs = request.start_ms > 0 ? (nowMs - request.start_ms) : -1;
        if (request.timeout_timer) {
            request.timeout_timer->stop();
            request.timeout_timer->deleteLater();
        }
        LogWarning("[ProtocolHandler] request aborted by disconnect, cmd=" << request.cmd.toStdString()
                   << ", request_id=" << pair.first.toStdString()
                   << ", cost_ms=" << costMs);
        if (request.callback_with_message) {
            request.callback_with_message(NETWORK_ERROR_CODE, QByteArray(), QString());
        } else if (request.callback) {
            request.callback(NETWORK_ERROR_CODE, QByteArray());
        }
    }
}
