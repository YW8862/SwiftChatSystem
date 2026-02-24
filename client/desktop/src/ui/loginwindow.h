#pragma once

#include <QWidget>

class WebSocketClient;
class ProtocolHandler;

/**
 * 登录窗口
 */
class LoginWindow : public QWidget {
    Q_OBJECT

public:
    explicit LoginWindow(WebSocketClient *wsClient, ProtocolHandler *protocol,
                        QWidget *parent = nullptr);
    ~LoginWindow();

signals:
    void loginSuccess(const QString& userId, const QString& token);

private slots:
    void doConnect();
    void onConnected();
    void onDisconnected();
    void onHeartbeatResponse(int code, const QByteArray& payload);
    void onLoginClicked();
    void onRegisterClicked();

private:
    void sendHeartbeat();

    WebSocketClient *m_wsClient = nullptr;
    ProtocolHandler *m_protocol = nullptr;
};
