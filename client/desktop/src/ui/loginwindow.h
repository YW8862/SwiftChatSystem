#pragma once

#include <QWidget>
#include <QPixmap>
#include <memory>

class WebSocketClient;
class ProtocolHandler;
class QLineEdit;
class QLabel;
class QPushButton;
class QToolButton;
class QStackedWidget;
class QTimer;
class MainWindow;
namespace client { class AppService; }

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
    QString currentServerUrl() const;
    bool ensureConnectedForRequest(const QString& connectingStatus);
    void submitLoginRequest(const QString& username, const QString& password);
    void sendConnectionProbe();
    void sendValidateTokenProbe(const QString& token);
    void sendHeartbeat();
    bool hasSavedSession() const;
    void scheduleAutoReconnect();
    void cancelAutoReconnect();
    void setLoginUiEnabled(bool enabled);
    void showDisconnectedState(const QString& reason);
    void showLoginState();
    void startRefreshAnimation();
    void stopRefreshAnimation();

    WebSocketClient *m_wsClient = nullptr;
    ProtocolHandler *m_protocol = nullptr;
    QStackedWidget* m_stacked = nullptr;
    QWidget* m_disconnectedPage = nullptr;
    QLabel* m_disconnectedReasonLabel = nullptr;
    QToolButton* m_refreshBtn = nullptr;
    QTimer* m_refreshSpinTimer = nullptr;
    QTimer* m_reconnectTimer = nullptr;
    QPixmap m_refreshIcon;
    int m_refreshAngle = 0;
    int m_reconnectIntervalMs = 3000;
    QWidget* m_loginPage = nullptr;
    QLineEdit* m_serverEdit = nullptr;
    QLineEdit* m_userEdit = nullptr;
    QLineEdit* m_passEdit = nullptr;
    QLabel* m_statusLabel = nullptr;
    QPushButton* m_loginBtn = nullptr;
    QPushButton* m_registerBtn = nullptr;
    bool m_loginInFlight = false;
    bool m_registerInFlight = false;
    bool m_loginPendingAfterConnect = false;
    QString m_pendingLoginUsername;
    QString m_pendingLoginPassword;
    std::unique_ptr<client::AppService> m_appService;
    std::unique_ptr<MainWindow> m_mainWindow;
};
