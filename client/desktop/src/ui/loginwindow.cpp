#include "loginwindow.h"
#include "network/websocket_client.h"
#include "network/protocol_handler.h"
#include "utils/settings.h"
#include "gate.pb.h"

#include <QDateTime>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

LoginWindow::LoginWindow(WebSocketClient *wsClient, ProtocolHandler *protocol,
                         QWidget *parent)
    : QWidget(parent), m_wsClient(wsClient), m_protocol(protocol) {
    setWindowTitle("SwiftChat - 登录");
    setFixedSize(400, 320);

    auto *layout = new QVBoxLayout(this);

    auto *statusLabel = new QLabel("连接中...");
    statusLabel->setObjectName("statusLabel");
    layout->addWidget(statusLabel);

    // 登录区（占位）
    layout->addWidget(new QLabel("用户名:"));
    auto *userEdit = new QLineEdit();
    layout->addWidget(userEdit);
    layout->addWidget(new QLabel("密码:"));
    auto *passEdit = new QLineEdit();
    passEdit->setEchoMode(QLineEdit::Password);
    layout->addWidget(passEdit);

    auto *btnLayout = new QHBoxLayout();
    auto *loginBtn = new QPushButton("登录");
    auto *regBtn = new QPushButton("注册");
    btnLayout->addWidget(loginBtn);
    btnLayout->addWidget(regBtn);
    layout->addLayout(btnLayout);

    connect(loginBtn, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
    connect(regBtn, &QPushButton::clicked, this, &LoginWindow::onRegisterClicked);

    if (m_wsClient) {
        connect(m_wsClient, &WebSocketClient::connected, this, &LoginWindow::onConnected);
        connect(m_wsClient, &WebSocketClient::disconnected, this, &LoginWindow::onDisconnected);
        // 延迟连接，确保窗口与事件循环就绪
        QMetaObject::invokeMethod(this, "doConnect", Qt::QueuedConnection);
    }
}

LoginWindow::~LoginWindow() = default;

void LoginWindow::doConnect() {
    if (m_wsClient) {
        m_wsClient->connect("ws://117.72.44.96:9090/ws");
    }
}

void LoginWindow::onConnected() {
    if (findChild<QLabel*>("statusLabel")) {
        findChild<QLabel*>("statusLabel")->setText("已连接，验证中...");
    }
    sendHeartbeat();
}

void LoginWindow::onDisconnected() {
    if (findChild<QLabel*>("statusLabel")) {
        findChild<QLabel*>("statusLabel")->setText("已断开");
    }
}

void LoginWindow::sendHeartbeat() {
    if (!m_protocol) return;

    swift::gate::HeartbeatRequest req;
    req.set_client_time(static_cast<int64_t>(QDateTime::currentMSecsSinceEpoch()));

    std::string payload;
    if (!req.SerializeToString(&payload)) return;

    m_protocol->sendRequest("heartbeat",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this](int code, const QByteArray& data) {
        onHeartbeatResponse(code, data);
    });
}

void LoginWindow::onHeartbeatResponse(int code, const QByteArray& payload) {
    auto *label = findChild<QLabel*>("statusLabel");
    if (!label) return;

    if (code == 0) {
        swift::gate::HeartbeatResponse resp;
        if (resp.ParseFromArray(payload.data(), payload.size())) {
            label->setText(QString("已连接 (server_time=%1)").arg(resp.server_time()));
        } else {
            label->setText("已连接");
        }
    } else {
        label->setText(QString("请求失败 code=%1").arg(code));
    }
}

void LoginWindow::onLoginClicked() {
    // TODO: 实现 auth.login
}

void LoginWindow::onRegisterClicked() {
    // TODO: 实现注册
}
