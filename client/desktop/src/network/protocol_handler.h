#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QTimer>
#include <functional>
#include <map>

/**
 * 协议处理器
 * 
 * 负责 Protobuf 消息的序列化/反序列化和命令分发
 */
class ProtocolHandler : public QObject {
    Q_OBJECT
    
public:
    using ResponseCallback = std::function<void(int code, const QByteArray& payload)>;
    using ResponseCallbackWithMessage = std::function<void(int code, const QByteArray& payload, const QString& message)>;
    
    explicit ProtocolHandler(QObject *parent = nullptr);
    ~ProtocolHandler();
    
    // 发送请求（异步，带回调）
    void sendRequest(const QString& cmd, const QByteArray& payload, 
                     ResponseCallback callback = nullptr);
    void sendRequest(const QString& cmd, const QByteArray& payload,
                     ResponseCallbackWithMessage callback);
    
    // 处理收到的消息
    void handleMessage(const QByteArray& data);

    // 断线时调用：将所有 pending 回调为网络错误并清空
    void clearPendingRequests();
    
signals:
    // 服务端推送的通知
    void newMessageNotify(const QByteArray& payload);
    void recallNotify(const QByteArray& payload);
    void readReceiptNotify(const QByteArray& payload);
    void friendRequestNotify(const QByteArray& payload);
    void friendAcceptedNotify(const QByteArray& payload);
    void userStatusChangeNotify(const QByteArray& payload);
    void kickedNotify(const QString& reason);
    
    // 需要发送数据
    void dataToSend(const QByteArray& data);
    
private:
    struct PendingRequest {
        ResponseCallback callback;
        ResponseCallbackWithMessage callback_with_message;
        QTimer* timeout_timer = nullptr;
        QString cmd;
        qint64 start_ms = 0;
    };

    void sendRequestImpl(const QString& cmd, const QByteArray& payload,
                         ResponseCallback callback,
                         ResponseCallbackWithMessage callbackWithMessage);

    QString generateRequestId();

    std::map<QString, PendingRequest> m_pendingRequests;
    int m_requestIdCounter = 0;
    int m_requestTimeoutMs = 15000;
};
