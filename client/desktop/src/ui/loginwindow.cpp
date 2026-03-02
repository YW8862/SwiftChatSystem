#include "loginwindow.h"
#include "mainwindow.h"
#include "network/websocket_client.h"
#include "network/protocol_handler.h"
#include "utils/settings.h"
#include "gate.pb.h"
#include "zone.pb.h"

#include <QDateTime>
#include <QHostInfo>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFont>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSysInfo>

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

    m_statusLabel = new QLabel("连接中...");
    m_statusLabel->setObjectName("statusLabel");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("color: #666; font-size: 13px;");
    mainLayout->addWidget(m_statusLabel);

    mainLayout->addSpacing(8);

    // 表单区
    auto *userLabel = new QLabel("用户名");
    userLabel->setStyleSheet("color: #333; font-weight: 500;");
    mainLayout->addWidget(userLabel);
    m_userEdit = new QLineEdit();
    m_userEdit->setPlaceholderText("请输入用户名");
    m_userEdit->setMinimumHeight(40);
    m_userEdit->setStyleSheet(
        "QLineEdit { padding: 8px 12px; border: 1px solid #ccc; border-radius: 6px; font-size: 14px; }"
        "QLineEdit:focus { border-color: #1a73e8; }"
    );
    mainLayout->addWidget(m_userEdit);

    mainLayout->addSpacing(4);

    auto *passLabel = new QLabel("密码");
    passLabel->setStyleSheet("color: #333; font-weight: 500;");
    mainLayout->addWidget(passLabel);
    m_passEdit = new QLineEdit();
    m_passEdit->setEchoMode(QLineEdit::Password);
    m_passEdit->setPlaceholderText("请输入密码");
    m_passEdit->setMinimumHeight(40);
    m_passEdit->setStyleSheet(
        "QLineEdit { padding: 8px 12px; border: 1px solid #ccc; border-radius: 6px; font-size: 14px; }"
        "QLineEdit:focus { border-color: #1a73e8; }"
    );
    mainLayout->addWidget(m_passEdit);

    mainLayout->addSpacing(16);

    // 按钮区
    auto *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);
    m_loginBtn = new QPushButton("登录");
    m_loginBtn->setMinimumHeight(44);
    m_loginBtn->setMinimumWidth(120);
    m_loginBtn->setStyleSheet(
        "QPushButton { background-color: #1a73e8; color: white; border: none; border-radius: 8px; font-size: 14px; font-weight: 500; }"
        "QPushButton:hover { background-color: #1557b0; }"
        "QPushButton:pressed { background-color: #0d47a1; }"
    );
    m_registerBtn = new QPushButton("注册");
    m_registerBtn->setMinimumHeight(44);
    m_registerBtn->setMinimumWidth(100);
    m_registerBtn->setStyleSheet(
        "QPushButton { background-color: #f1f3f4; color: #333; border: 1px solid #dadce0; border-radius: 8px; font-size: 14px; }"
        "QPushButton:hover { background-color: #e8eaed; }"
        "QPushButton:pressed { background-color: #dadce0; }"
    );
    btnLayout->addStretch();
    btnLayout->addWidget(m_loginBtn);
    btnLayout->addWidget(m_registerBtn);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);

    connect(m_loginBtn, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
    connect(m_registerBtn, &QPushButton::clicked, this, &LoginWindow::onRegisterClicked);

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
        m_wsClient->connect(Settings::instance().serverUrl());
    }
}

void LoginWindow::onConnected() {
    if (m_statusLabel) m_statusLabel->setText("已连接");
    sendHeartbeat();
}

void LoginWindow::onDisconnected() {
    if (m_statusLabel) m_statusLabel->setText("已断开");
    setLoginUiEnabled(true);
    m_loginInFlight = false;
}

void LoginWindow::onConnectionError(const QString& error) {
    if (m_statusLabel) m_statusLabel->setText("连接失败: " + error);
    QString detail = error;
    if (error.contains("ConnectionRefused", Qt::CaseInsensitive) || error.contains("请求被拒绝")) {
        detail += "\n\n常见原因：\n"
                  "• 服务(GateSvr)未启动或未监听 9090\n"
                  "• 防火墙拒绝该端口\n"
                  "• 网关/路由器需开放 9090\n"
                  "• Minikube：确认服务已暴露(NodePort/port-forward)";
    }
    QMessageBox::warning(this, "连接失败", "无法连接到服务器 " + Settings::instance().serverUrl() + "\n\n" + detail);
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
    if (!m_statusLabel) return;

    if (code == 0) {
        swift::gate::HeartbeatResponse resp;
        if (resp.ParseFromArray(payload.data(), payload.size())) {
            m_statusLabel->setText(QString("已连接 (server_time=%1)").arg(resp.server_time()));
        } else {
            m_statusLabel->setText("已连接");
        }
    } else {
        m_statusLabel->setText(QString("请求失败 code=%1").arg(code));
    }
}

void LoginWindow::onLoginClicked() {
    if (!m_wsClient || !m_protocol || !m_userEdit || !m_passEdit) return;
    if (m_loginInFlight) return;

    const QString username = m_userEdit->text().trimmed();
    const QString password = m_passEdit->text();
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "登录失败", "用户名和密码不能为空。");
        return;
    }
    if (!m_wsClient->isConnected()) {
        QMessageBox::warning(this, "登录失败", "当前未连接到网关，请稍后重试。");
        return;
    }

    swift::zone::AuthLoginPayload req;
    req.set_username(username.toStdString());
    req.set_password(password.toStdString());
    QByteArray machineId = QSysInfo::machineUniqueId();
    const QString deviceId = machineId.isEmpty()
        ? QHostInfo::localHostName()
        : QString::fromUtf8(machineId.toHex());
    req.set_device_id(deviceId.toStdString());
    req.set_device_type("desktop");

    std::string payload;
    if (!req.SerializeToString(&payload)) {
        QMessageBox::warning(this, "登录失败", "请求序列化失败。");
        return;
    }

    m_loginInFlight = true;
    setLoginUiEnabled(false);
    if (m_statusLabel) m_statusLabel->setText("登录中...");

    m_protocol->sendRequest("auth.login",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this](int code, const QByteArray& data) {
        m_loginInFlight = false;
        setLoginUiEnabled(true);

        if (code != 0) {
            if (m_statusLabel) m_statusLabel->setText(QString("登录失败 code=%1").arg(code));
            QMessageBox::warning(this, "登录失败", QString("服务器返回错误码：%1").arg(code));
            return;
        }

        swift::zone::AuthLoginResponsePayload resp;
        if (!resp.ParseFromArray(data.data(), data.size())) {
            if (m_statusLabel) m_statusLabel->setText("登录失败：响应解析失败");
            QMessageBox::warning(this, "登录失败", "登录响应解析失败。");
            return;
        }
        if (!resp.success() || resp.user_id().empty() || resp.token().empty()) {
            const QString err = resp.error().empty()
                ? QStringLiteral("用户名或密码错误")
                : QString::fromStdString(resp.error());
            if (m_statusLabel) m_statusLabel->setText("登录失败");
            QMessageBox::warning(this, "登录失败", err);
            return;
        }

        const QString userId = QString::fromStdString(resp.user_id());
        const QString token = QString::fromStdString(resp.token());
        Settings::instance().saveLoginInfo(userId, token);
        emit loginSuccess(userId, token);

        if (m_statusLabel) m_statusLabel->setText("登录成功");
        if (!m_mainWindow) {
            m_mainWindow = std::make_unique<MainWindow>(m_protocol, userId);
        }
        m_mainWindow->show();
        hide();
    });
}

void LoginWindow::onRegisterClicked() {
    if (!m_wsClient || !m_protocol) return;
    if (m_loginInFlight || m_registerInFlight) return;
    if (!m_wsClient->isConnected()) {
        QMessageBox::warning(this, "注册失败", "当前未连接到网关，请稍后重试。");
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("注册账号");
    dialog.setModal(true);
    auto* form = new QFormLayout(&dialog);

    auto* usernameEdit = new QLineEdit(&dialog);
    usernameEdit->setPlaceholderText("3-32位，字母/数字/下划线");
    auto* passwordEdit = new QLineEdit(&dialog);
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setPlaceholderText("至少8位");
    auto* confirmEdit = new QLineEdit(&dialog);
    confirmEdit->setEchoMode(QLineEdit::Password);
    confirmEdit->setPlaceholderText("再次输入密码");
    auto* nicknameEdit = new QLineEdit(&dialog);
    nicknameEdit->setPlaceholderText("可选，默认同用户名");
    auto* emailEdit = new QLineEdit(&dialog);
    emailEdit->setPlaceholderText("可选");

    form->addRow("用户名", usernameEdit);
    form->addRow("密码", passwordEdit);
    form->addRow("确认密码", confirmEdit);
    form->addRow("昵称", nicknameEdit);
    form->addRow("邮箱", emailEdit);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText("注册");
    buttons->button(QDialogButtonBox::Cancel)->setText("取消");
    form->addRow(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    usernameEdit->setText(m_userEdit ? m_userEdit->text().trimmed() : QString());
    dialog.resize(360, 260);
    if (dialog.exec() != QDialog::Accepted) return;

    const QString username = usernameEdit->text().trimmed();
    const QString password = passwordEdit->text();
    const QString confirm = confirmEdit->text();
    const QString nickname = nicknameEdit->text().trimmed();
    const QString email = emailEdit->text().trimmed();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "注册失败", "用户名和密码不能为空。");
        return;
    }
    if (password != confirm) {
        QMessageBox::warning(this, "注册失败", "两次输入的密码不一致。");
        return;
    }

    swift::zone::AuthRegisterPayload req;
    req.set_username(username.toStdString());
    req.set_password(password.toStdString());
    if (!nickname.isEmpty()) req.set_nickname(nickname.toStdString());
    if (!email.isEmpty()) req.set_email(email.toStdString());

    std::string payload;
    if (!req.SerializeToString(&payload)) {
        QMessageBox::warning(this, "注册失败", "请求序列化失败。");
        return;
    }

    m_registerInFlight = true;
    setLoginUiEnabled(false);
    if (m_statusLabel) m_statusLabel->setText("注册中...");
    m_protocol->sendRequest("auth.register",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this, username](int code, const QByteArray& data) {
        m_registerInFlight = false;
        setLoginUiEnabled(true);

        if (code != 0) {
            swift::zone::AuthRegisterResponsePayload resp;
            QString err = QString("服务器返回错误码：%1").arg(code);
            if (resp.ParseFromArray(data.data(), data.size()) && !resp.error().empty()) {
                err = QString::fromStdString(resp.error());
            }
            if (m_statusLabel) m_statusLabel->setText(QString("注册失败 code=%1").arg(code));
            QMessageBox::warning(this, "注册失败", err);
            return;
        }

        swift::zone::AuthRegisterResponsePayload resp;
        if (!resp.ParseFromArray(data.data(), data.size())) {
            if (m_statusLabel) m_statusLabel->setText("注册失败：响应解析失败");
            QMessageBox::warning(this, "注册失败", "注册响应解析失败。");
            return;
        }
        if (!resp.success()) {
            const QString err = resp.error().empty()
                ? QStringLiteral("注册失败")
                : QString::fromStdString(resp.error());
            if (m_statusLabel) m_statusLabel->setText("注册失败");
            QMessageBox::warning(this, "注册失败", err);
            return;
        }

        if (m_userEdit) m_userEdit->setText(username);
        if (m_statusLabel) m_statusLabel->setText("注册成功，请登录");
        QMessageBox::information(this, "注册成功", "账号已创建，请使用该账号登录。");
    });
}

void LoginWindow::setLoginUiEnabled(bool enabled) {
    if (m_userEdit) m_userEdit->setEnabled(enabled);
    if (m_passEdit) m_passEdit->setEnabled(enabled);
    if (m_loginBtn) m_loginBtn->setEnabled(enabled);
    if (m_registerBtn) m_registerBtn->setEnabled(enabled);
}
