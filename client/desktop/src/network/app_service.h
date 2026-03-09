#pragma once

#include <QString>
#include <functional>

#include "zone.pb.h"

class ProtocolHandler;

namespace client {

class AppService {
public:
    using AuthLoginCallback = std::function<void(
        int code,
        const QString& message,
        const swift::zone::AuthLoginResponsePayload& payload
    )>;

    using ChatGetHistoryCallback = std::function<void(
        int code,
        const QString& message,
        const swift::zone::ChatGetHistoryResponsePayload& payload
    )>;

    explicit AppService(ProtocolHandler* protocol);

    void sendAuthLogin(const QString& username,
                       const QString& password,
                       const QString& deviceId,
                       const QString& deviceType,
                       AuthLoginCallback callback) const;

    void sendChatGetHistory(const QString& chatId,
                            int chatType,
                            const QString& beforeMsgId,
                            int limit,
                            ChatGetHistoryCallback callback) const;

private:
    ProtocolHandler* m_protocol = nullptr;
};

}  // namespace client
