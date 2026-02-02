#pragma once

#include <QObject>
#include <QByteArray>
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
    
    explicit ProtocolHandler(QObject *parent = nullptr);
    ~ProtocolHandler();
    
    // 发送请求（异步，带回调）
    void sendRequest(const QString& cmd, const QByteArray& payload, 
                     ResponseCallback callback = nullptr);
    
    // 处理收到的消息
    void handleMessage(const QByteArray& data);
    
signals:
    // 服务端推送的通知
    void newMessageNotify(const QByteArray& payload);
    void recallNotify(const QByteArray& payload);
    void readReceiptNotify(const QByteArray& payload);
    void friendRequestNotify(const QByteArray& payload);
    void userStatusChangeNotify(const QByteArray& payload);
    void kickedNotify(const QString& reason);
    
    // 需要发送数据
    void dataToSend(const QByteArray& data);
    
private:
    QString generateRequestId();
    
    std::map<QString, ResponseCallback> m_pendingRequests;
    int m_requestIdCounter = 0;
};
