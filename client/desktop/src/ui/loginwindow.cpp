#include "loginwindow.h"
#include "mainwindow.h"
#include "network/websocket_client.h"
#include "network/protocol_handler.h"
#include "utils/settings.h"
#include "gate.pb.h"
#include "zone.pb.h"

#include <QDateTime>
#include <QFrame>
#include <QHostInfo>
#include <QIcon>
#include <QMessageBox>
#include <QTimer>
#include <QTransform>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFont>
#include <QStackedWidget>
#include <QToolButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSysInfo>

LoginWindow::LoginWindow(WebSocketClient *wsClient, ProtocolHandler *protocol,
                         QWidget *parent)
    : QWidget(parent), m_wsClient(wsClient), m_protocol(protocol) {
    setWindowTitle("SwiftChat - 登录");
    setFixedSize(460, 540);
    setMinimumWidth(420);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(22, 22, 22, 22);
    mainLayout->setSpacing(0);

    m_stacked = new QStackedWidget(this);
    mainLayout->addWidget(m_stacked);

    m_disconnectedPage = new QWidget(m_stacked);
    auto* disconnectedLayout = new QVBoxLayout(m_disconnectedPage);
    disconnectedLayout->setContentsMargins(12, 16, 12, 16);
    disconnectedLayout->setSpacing(14);
    disconnectedLayout->addStretch();

    auto* disconnectedCard = new QFrame(m_disconnectedPage);
    disconnectedCard->setObjectName("disconnectCard");
    auto* disconnectedCardLayout = new QVBoxLayout(disconnectedCard);
    disconnectedCardLayout->setContentsMargins(28, 26, 28, 24);
    disconnectedCardLayout->setSpacing(12);

    auto* disconnectedTitle = new QLabel("服务器连接已断开", disconnectedCard);
    disconnectedTitle->setObjectName("disconnectTitle");
    disconnectedTitle->setAlignment(Qt::AlignCenter);
    disconnectedCardLayout->addWidget(disconnectedTitle);

    m_disconnectedReasonLabel = new QLabel("当前无法连接到服务器", disconnectedCard);
    m_disconnectedReasonLabel->setObjectName("disconnectReason");
    m_disconnectedReasonLabel->setAlignment(Qt::AlignCenter);
    m_disconnectedReasonLabel->setWordWrap(true);
    disconnectedCardLayout->addWidget(m_disconnectedReasonLabel);

    m_refreshBtn = new QToolButton(disconnectedCard);
    m_refreshBtn->setObjectName("refreshButton");
    m_refreshIcon = QPixmap(":/icons/reflash.png");
    m_refreshBtn->setIcon(QIcon(m_refreshIcon));
    m_refreshBtn->setIconSize(QSize(54, 54));
    m_refreshBtn->setToolTip("刷新并重连");
    m_refreshBtn->setCursor(Qt::PointingHandCursor);
    m_refreshBtn->setMinimumSize(96, 96);
    disconnectedCardLayout->addWidget(m_refreshBtn, 0, Qt::AlignHCenter);

    auto* disconnectedHint = new QLabel("点击图标尝试重新连接", disconnectedCard);
    disconnectedHint->setObjectName("disconnectHint");
    disconnectedHint->setAlignment(Qt::AlignCenter);
    disconnectedCardLayout->addWidget(disconnectedHint);

    disconnectedLayout->addWidget(disconnectedCard);
    disconnectedLayout->addStretch();

    m_loginPage = new QWidget(m_stacked);
    auto* loginPageLayout = new QVBoxLayout(m_loginPage);
    loginPageLayout->setContentsMargins(0, 0, 0, 0);
    loginPageLayout->setSpacing(0);
    loginPageLayout->addStretch();

    auto *card = new QFrame(m_loginPage);
    card->setObjectName("loginCard");
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(28, 26, 28, 24);
    cardLayout->setSpacing(12);

    auto *titleLabel = new QLabel("SwiftChat", card);
    titleLabel->setObjectName("loginTitle");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(24);
    titleFont.setWeight(QFont::Bold);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(titleLabel);

    auto *subtitleLabel = new QLabel("欢迎回来，登录后即可开始聊天", card);
    subtitleLabel->setObjectName("loginSubtitle");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(subtitleLabel);

    cardLayout->addSpacing(6);

    m_statusLabel = new QLabel("连接中...", card);
    m_statusLabel->setObjectName("statusLabel");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(m_statusLabel);

    cardLayout->addSpacing(10);

    auto *userLabel = new QLabel("用户名", card);
    userLabel->setObjectName("fieldLabel");
    cardLayout->addWidget(userLabel);
    m_userEdit = new QLineEdit(card);
    m_userEdit->setPlaceholderText("请输入用户名");
    m_userEdit->setMinimumHeight(42);
    cardLayout->addWidget(m_userEdit);

    cardLayout->addSpacing(2);

    auto *passLabel = new QLabel("密码", card);
    passLabel->setObjectName("fieldLabel");
    cardLayout->addWidget(passLabel);
    m_passEdit = new QLineEdit(card);
    m_passEdit->setEchoMode(QLineEdit::Password);
    m_passEdit->setPlaceholderText("请输入密码");
    m_passEdit->setMinimumHeight(42);
    cardLayout->addWidget(m_passEdit);

    cardLayout->addSpacing(16);

    auto *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    m_loginBtn = new QPushButton("登录", card);
    m_loginBtn->setObjectName("primaryBtn");
    m_loginBtn->setMinimumHeight(44);
    m_loginBtn->setMinimumWidth(140);
    m_registerBtn = new QPushButton("注册", card);
    m_registerBtn->setObjectName("secondaryBtn");
    m_registerBtn->setMinimumHeight(44);
    m_registerBtn->setMinimumWidth(108);
    btnLayout->addWidget(m_loginBtn);
    btnLayout->addWidget(m_registerBtn);
    cardLayout->addLayout(btnLayout);

    cardLayout->addSpacing(4);

    auto *tipLabel = new QLabel("首次使用请先注册账号", card);
    tipLabel->setObjectName("tipLabel");
    tipLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(tipLabel);

    loginPageLayout->addWidget(card);
    loginPageLayout->addStretch();

    m_stacked->addWidget(m_disconnectedPage);
    m_stacked->addWidget(m_loginPage);

    setStyleSheet(
        "LoginWindow { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #edf3ff, stop:1 #d8e7ff); }"
        "QFrame#loginCard { background: #ffffff; border: 1px solid #dbe5f5; border-radius: 18px; }"
        "QLabel#loginTitle { color: #1f2f4a; }"
        "QLabel#loginSubtitle { color: #6b7790; font-size: 13px; }"
        "QLabel#statusLabel { color: #4f6ea8; font-size: 13px; background: #eef4ff; border: 1px solid #d5e4ff; border-radius: 10px; padding: 6px 8px; }"
        "QLabel#fieldLabel { color: #2f3b50; font-size: 13px; font-weight: 600; }"
        "QLabel#tipLabel { color: #8a95ab; font-size: 12px; }"
        "QLineEdit { background: #f9fbff; border: 1px solid #d4ddee; border-radius: 10px; padding: 0 12px; font-size: 14px; }"
        "QLineEdit:focus { border-color: #6d9fff; background: #ffffff; }"
        "QPushButton#primaryBtn { background: #3f8cff; color: white; border: none; border-radius: 10px; font-size: 14px; font-weight: 600; }"
        "QPushButton#primaryBtn:hover { background: #2f7df7; }"
        "QPushButton#primaryBtn:pressed { background: #1f6cdf; }"
        "QPushButton#primaryBtn:disabled { background: #a9c7f7; color: #ecf4ff; }"
        "QPushButton#secondaryBtn { background: #ffffff; color: #31558f; border: 1px solid #c8d7f3; border-radius: 10px; font-size: 14px; font-weight: 600; }"
        "QPushButton#secondaryBtn:hover { background: #f5f9ff; border-color: #9ebcf0; }"
        "QPushButton#secondaryBtn:pressed { background: #eaf1ff; }"
        "QPushButton#secondaryBtn:disabled { color: #9caac2; border-color: #d3dbe8; }"
        "QFrame#disconnectCard { background: #ffffff; border: 1px solid #dbe5f5; border-radius: 18px; }"
        "QLabel#disconnectTitle { color: #2b3d5d; font-size: 20px; font-weight: 700; }"
        "QLabel#disconnectReason { color: #60718f; font-size: 13px; line-height: 1.4; background: #f3f7ff; border: 1px solid #dde9ff; border-radius: 10px; padding: 10px 12px; }"
        "QLabel#disconnectHint { color: #8192ad; font-size: 12px; }"
        "QToolButton#refreshButton { background: #ebf3ff; border: 1px solid #c9ddff; border-radius: 48px; padding: 10px; }"
        "QToolButton#refreshButton:hover { background: #dcecff; border-color: #9fc2ff; }"
        "QToolButton#refreshButton:pressed { background: #cadfff; }"
        "QToolButton#refreshButton:disabled { background: #eef2f8; border-color: #d8e0eb; }"
    );

    connect(m_loginBtn, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
    connect(m_registerBtn, &QPushButton::clicked, this, &LoginWindow::onRegisterClicked);
    connect(m_passEdit, &QLineEdit::returnPressed, this, &LoginWindow::onLoginClicked);
    connect(m_refreshBtn, &QToolButton::clicked, this, &LoginWindow::doConnect);
    m_refreshSpinTimer = new QTimer(this);
    m_refreshSpinTimer->setInterval(45);
    connect(m_refreshSpinTimer, &QTimer::timeout, this, [this]() {
        if (!m_refreshBtn || m_refreshIcon.isNull()) return;
        m_refreshAngle = (m_refreshAngle + 18) % 360;
        const QPixmap rotated = m_refreshIcon.transformed(QTransform().rotate(m_refreshAngle), Qt::SmoothTransformation);
        m_refreshBtn->setIcon(QIcon(rotated));
    });

    if (m_wsClient) {
        connect(m_wsClient, &WebSocketClient::connected, this, &LoginWindow::onConnected);
        connect(m_wsClient, &WebSocketClient::disconnected, this, &LoginWindow::onDisconnected);
        connect(m_wsClient, &WebSocketClient::errorOccurred, this, &LoginWindow::onConnectionError);
        // 延迟连接，确保窗口与事件循环就绪
        QMetaObject::invokeMethod(this, "doConnect", Qt::QueuedConnection);
    }

    showDisconnectedState("正在连接服务器...");
}

LoginWindow::~LoginWindow() = default;

void LoginWindow::doConnect() {
    if (m_wsClient) {
        showDisconnectedState("正在尝试连接服务器...");
        if (m_refreshBtn) m_refreshBtn->setEnabled(false);
        startRefreshAnimation();
        m_wsClient->connect(Settings::instance().serverUrl());
    }
}

void LoginWindow::onConnected() {
    stopRefreshAnimation();
    if (m_refreshBtn) m_refreshBtn->setEnabled(true);
    showLoginState();
    if (m_statusLabel) m_statusLabel->setText("已连接");
    sendHeartbeat();
}

void LoginWindow::onDisconnected() {
    stopRefreshAnimation();
    m_loginInFlight = false;
    m_registerInFlight = false;
    setLoginUiEnabled(true);
    if (m_refreshBtn) m_refreshBtn->setEnabled(true);
    showDisconnectedState("与服务器的连接已断开，请点击刷新图标重连。");
}

void LoginWindow::onConnectionError(const QString& error) {
    stopRefreshAnimation();
    QString detail = error;
    if (error.contains("ConnectionRefused", Qt::CaseInsensitive) || error.contains("请求被拒绝")) {
        detail += "\n\n常见原因：\n"
                  "• 服务(GateSvr)未启动或未监听 9090\n"
                  "• 防火墙拒绝该端口\n"
                  "• 网关/路由器需开放 9090\n"
                  "• Minikube：确认服务已暴露(NodePort/port-forward)";
    }
    m_loginInFlight = false;
    m_registerInFlight = false;
    setLoginUiEnabled(true);
    if (m_refreshBtn) m_refreshBtn->setEnabled(true);
    showDisconnectedState("连接失败：\n" + detail);
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
    usernameEdit->setMinimumHeight(38);
    auto* passwordEdit = new QLineEdit(&dialog);
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setPlaceholderText("至少8位");
    passwordEdit->setMinimumHeight(38);
    auto* confirmEdit = new QLineEdit(&dialog);
    confirmEdit->setEchoMode(QLineEdit::Password);
    confirmEdit->setPlaceholderText("再次输入密码");
    confirmEdit->setMinimumHeight(38);
    auto* nicknameEdit = new QLineEdit(&dialog);
    nicknameEdit->setPlaceholderText("可选，默认同用户名");
    nicknameEdit->setMinimumHeight(38);
    auto* emailEdit = new QLineEdit(&dialog);
    emailEdit->setPlaceholderText("可选");
    emailEdit->setMinimumHeight(38);

    form->addRow("用户名", usernameEdit);
    form->addRow("密码", passwordEdit);
    form->addRow("确认密码", confirmEdit);
    form->addRow("昵称", nicknameEdit);
    form->addRow("邮箱", emailEdit);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText("注册");
    buttons->button(QDialogButtonBox::Cancel)->setText("取消");
    buttons->button(QDialogButtonBox::Ok)->setObjectName("registerOkBtn");
    buttons->button(QDialogButtonBox::Cancel)->setObjectName("registerCancelBtn");
    form->addRow(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    usernameEdit->setText(m_userEdit ? m_userEdit->text().trimmed() : QString());
    dialog.resize(410, 330);
    dialog.setStyleSheet(
        "QDialog { background: #f8fbff; }"
        "QLabel { color: #2c3a50; font-size: 13px; font-weight: 600; }"
        "QLineEdit { background: #ffffff; border: 1px solid #d4ddee; border-radius: 9px; padding: 0 10px; font-size: 13px; }"
        "QLineEdit:focus { border-color: #6d9fff; }"
        "QPushButton#registerOkBtn { background: #3f8cff; color: white; border: none; border-radius: 9px; min-height: 34px; padding: 0 16px; font-weight: 600; }"
        "QPushButton#registerOkBtn:hover { background: #2f7df7; }"
        "QPushButton#registerCancelBtn { background: #ffffff; color: #30538d; border: 1px solid #cad8f2; border-radius: 9px; min-height: 34px; padding: 0 16px; font-weight: 600; }"
        "QPushButton#registerCancelBtn:hover { background: #f3f8ff; }"
    );
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

void LoginWindow::showDisconnectedState(const QString& reason) {
    if (m_disconnectedReasonLabel) {
        m_disconnectedReasonLabel->setText(
            reason.isEmpty() ? QStringLiteral("当前无法连接到服务器。") : reason
        );
    }
    if (m_stacked && m_disconnectedPage) {
        m_stacked->setCurrentWidget(m_disconnectedPage);
    }
}

void LoginWindow::showLoginState() {
    if (m_stacked && m_loginPage) {
        m_stacked->setCurrentWidget(m_loginPage);
    }
}

void LoginWindow::startRefreshAnimation() {
    if (!m_refreshSpinTimer || !m_refreshBtn || m_refreshIcon.isNull()) return;
    if (!m_refreshSpinTimer->isActive()) {
        m_refreshSpinTimer->start();
    }
}

void LoginWindow::stopRefreshAnimation() {
    if (m_refreshSpinTimer && m_refreshSpinTimer->isActive()) {
        m_refreshSpinTimer->stop();
    }
    m_refreshAngle = 0;
    if (m_refreshBtn && !m_refreshIcon.isNull()) {
        m_refreshBtn->setIcon(QIcon(m_refreshIcon));
    }
}
