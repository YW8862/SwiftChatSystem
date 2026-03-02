#pragma once

#include <QWidget>
#include <memory>

class WebSocketClient;
class ProtocolHandler;
class QLineEdit;
class QLabel;
class QPushButton;
class MainWindow;

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
    void onConnectionError(const QString& error);
    void onHeartbeatResponse(int code, const QByteArray& payload);
    void onLoginClicked();
    void onRegisterClicked();

private:
    void sendHeartbeat();
    void setLoginUiEnabled(bool enabled);

    WebSocketClient *m_wsClient = nullptr;
    ProtocolHandler *m_protocol = nullptr;
    QLineEdit* m_userEdit = nullptr;
    QLineEdit* m_passEdit = nullptr;
    QLabel* m_statusLabel = nullptr;
    QPushButton* m_loginBtn = nullptr;
    QPushButton* m_registerBtn = nullptr;
    bool m_loginInFlight = false;
    bool m_registerInFlight = false;
    std::unique_ptr<MainWindow> m_mainWindow;
};
