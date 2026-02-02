#pragma once

#include <QObject>
#include <QWebSocket>
#include <memory>

/**
 * WebSocket 客户端
 * 
 * 负责与 GateSvr 通信
 */
class WebSocketClient : public QObject {
    Q_OBJECT
    
public:
    explicit WebSocketClient(QObject *parent = nullptr);
    ~WebSocketClient();
    
    void connect(const QString& url);
    void disconnect();
    bool isConnected() const;
    
    void sendMessage(const QByteArray& data);
    
signals:
    void connected();
    void disconnected();
    void messageReceived(const QByteArray& data);
    void errorOccurred(const QString& error);
    
private slots:
    void onConnected();
    void onDisconnected();
    void onBinaryMessageReceived(const QByteArray& message);
    void onError(QAbstractSocket::SocketError error);
    
private:
    std::unique_ptr<QWebSocket> m_socket;
};
