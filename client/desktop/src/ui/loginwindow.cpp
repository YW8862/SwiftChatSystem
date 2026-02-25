#include "loginwindow.h"
#include "network/websocket_client.h"
#include "network/protocol_handler.h"
#include "utils/settings.h"
#include "gate.pb.h"

#include <QDateTime>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFont>

LoginWindow::LoginWindow(WebSocketClient *wsClient, ProtocolHandler *protocol,
                         QWidget *parent)
    : QWidget(parent), m_wsClient(wsClient), m_protocol(protocol) {
    setWindowTitle("SwiftChat - 登录");
    setFixedSize(420, 400);
    setMinimumWidth(380);

    const int margin = 32;
    const int spacing = 14;

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(margin, margin, margin, margin);
    mainLayout->setSpacing(spacing);

    // 标题区
    auto *titleLabel = new QLabel("SwiftChat");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(22);
    titleFont.setWeight(QFont::DemiBold);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    auto *statusLabel = new QLabel("连接中...");
    statusLabel->setObjectName("statusLabel");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color: #666; font-size: 13px;");
    mainLayout->addWidget(statusLabel);

    mainLayout->addSpacing(8);

    // 表单区
    auto *userLabel = new QLabel("用户名");
    userLabel->setStyleSheet("color: #333; font-weight: 500;");
    mainLayout->addWidget(userLabel);
    auto *userEdit = new QLineEdit();
    userEdit->setPlaceholderText("请输入用户名");
    userEdit->setMinimumHeight(40);
    userEdit->setStyleSheet(
        "QLineEdit { padding: 8px 12px; border: 1px solid #ccc; border-radius: 6px; font-size: 14px; }"
        "QLineEdit:focus { border-color: #1a73e8; }"
    );
    mainLayout->addWidget(userEdit);

    mainLayout->addSpacing(4);

    auto *passLabel = new QLabel("密码");
    passLabel->setStyleSheet("color: #333; font-weight: 500;");
    mainLayout->addWidget(passLabel);
    auto *passEdit = new QLineEdit();
    passEdit->setEchoMode(QLineEdit::Password);
    passEdit->setPlaceholderText("请输入密码");
    passEdit->setMinimumHeight(40);
    passEdit->setStyleSheet(
        "QLineEdit { padding: 8px 12px; border: 1px solid #ccc; border-radius: 6px; font-size: 14px; }"
        "QLineEdit:focus { border-color: #1a73e8; }"
    );
    mainLayout->addWidget(passEdit);

    mainLayout->addSpacing(16);

    // 按钮区
    auto *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);
    auto *loginBtn = new QPushButton("登录");
    loginBtn->setMinimumHeight(44);
    loginBtn->setMinimumWidth(120);
    loginBtn->setStyleSheet(
        "QPushButton { background-color: #1a73e8; color: white; border: none; border-radius: 8px; font-size: 14px; font-weight: 500; }"
        "QPushButton:hover { background-color: #1557b0; }"
        "QPushButton:pressed { background-color: #0d47a1; }"
    );
    auto *regBtn = new QPushButton("注册");
    regBtn->setMinimumHeight(44);
    regBtn->setMinimumWidth(100);
    regBtn->setStyleSheet(
        "QPushButton { background-color: #f1f3f4; color: #333; border: 1px solid #dadce0; border-radius: 8px; font-size: 14px; }"
        "QPushButton:hover { background-color: #e8eaed; }"
        "QPushButton:pressed { background-color: #dadce0; }"
    );
    btnLayout->addStretch();
    btnLayout->addWidget(loginBtn);
    btnLayout->addWidget(regBtn);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);

    connect(loginBtn, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
    connect(regBtn, &QPushButton::clicked, this, &LoginWindow::onRegisterClicked);

    if (m_wsClient) {
        connect(m_wsClient, &WebSocketClient::connected, this, &LoginWindow::onConnected);
        connect(m_wsClient, &WebSocketClient::disconnected, this, &LoginWindow::onDisconnected);
        connect(m_wsClient, &WebSocketClient::errorOccurred, this, &LoginWindow::onConnectionError);
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

void LoginWindow::onConnectionError(const QString& error) {
    QLabel *label = findChild<QLabel*>("statusLabel");
    if (label) {
        label->setText("连接失败: " + error);
    }
    QMessageBox::warning(this, "连接失败", "无法连接到服务器 ws://117.72.44.96:9090/ws\n\n" + error);
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
    QMessageBox::information(this, "登录", "登录功能开发中，请稍后。\n确保已连接服务器后再试。");
}

void LoginWindow::onRegisterClicked() {
    QMessageBox::information(this, "注册", "注册功能开发中，请稍后。");
}
