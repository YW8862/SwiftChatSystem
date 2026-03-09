#include "app_service.h"

#include "protocol_handler.h"

namespace client {

AppService::AppService(ProtocolHandler* protocol) : m_protocol(protocol) {}

void AppService::sendAuthLogin(const QString& username,
                               const QString& password,
                               const QString& deviceId,
                               const QString& deviceType,
                               AuthLoginCallback callback) const {
    swift::zone::AuthLoginResponsePayload emptyResp;
    if (!m_protocol) {
        if (callback) callback(-1, "network layer unavailable", emptyResp);
        return;
    }

    swift::zone::AuthLoginPayload req;
    req.set_username(username.toStdString());
    req.set_password(password.toStdString());
    req.set_device_id(deviceId.toStdString());
    req.set_device_type(deviceType.toStdString());

    std::string serialized;
    if (!req.SerializeToString(&serialized)) {
        if (callback) callback(-1, "auth.login serialize failed", emptyResp);
        return;
    }

    m_protocol->sendRequest(
        "auth.login",
        QByteArray(serialized.data(), static_cast<int>(serialized.size())),
        [callback](int code, const QByteArray& data, const QString& message) {
            swift::zone::AuthLoginResponsePayload resp;
            if (code != 0) {
                if (callback) callback(code, message, resp);
                return;
            }
            if (!resp.ParseFromArray(data.data(), data.size())) {
                if (callback) callback(-1, "auth.login response parse failed", resp);
                return;
            }
            if (callback) callback(code, QString::fromStdString(resp.error()), resp);
        }
    );
}

void AppService::sendChatGetHistory(const QString& chatId,
                                    int chatType,
                                    const QString& beforeMsgId,
                                    int limit,
                                    ChatGetHistoryCallback callback) const {
    swift::zone::ChatGetHistoryResponsePayload emptyResp;
    if (!m_protocol) {
        if (callback) callback(-1, "network layer unavailable", emptyResp);
        return;
    }

    swift::zone::ChatGetHistoryPayload req;
    req.set_chat_id(chatId.toStdString());
    req.set_chat_type(chatType);
    if (!beforeMsgId.trimmed().isEmpty()) {
        req.set_before_msg_id(beforeMsgId.toStdString());
    }
    req.set_limit(limit > 0 ? limit : 50);

    std::string serialized;
    if (!req.SerializeToString(&serialized)) {
        if (callback) callback(-1, "chat.get_history serialize failed", emptyResp);
        return;
    }

    m_protocol->sendRequest(
        "chat.get_history",
        QByteArray(serialized.data(), static_cast<int>(serialized.size())),
        [callback](int code, const QByteArray& data, const QString& message) {
            swift::zone::ChatGetHistoryResponsePayload resp;
            if (code != 0) {
                if (callback) callback(code, message, resp);
                return;
            }
            if (!resp.ParseFromArray(data.data(), data.size())) {
                if (callback) callback(-1, "chat.get_history response parse failed", resp);
                return;
            }
            if (callback) callback(code, QString(), resp);
        }
    );
}

}  // namespace client
