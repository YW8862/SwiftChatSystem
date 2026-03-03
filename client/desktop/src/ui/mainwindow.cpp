#include "mainwindow.h"

#include "contactwidget.h"
#include "chatwidget.h"
#include "network/protocol_handler.h"
#include "zone.pb.h"
#include "gate.pb.h"

#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(ProtocolHandler* protocol,
                       const QString& currentUserId,
                       QWidget *parent)
    : QMainWindow(parent)
    , m_protocol(protocol)
    , m_currentUserId(currentUserId) {
    setWindowTitle("SwiftChat");
    resize(1024, 768);

    setupUi();
    wireSignals();
    syncConversations();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    auto* central = new QWidget(this);
    central->setObjectName("mainRoot");
    auto* layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* splitter = new QSplitter(Qt::Horizontal, central);
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(6);
    layout->addWidget(splitter);

    auto* navPanel = new QFrame(central);
    navPanel->setObjectName("navPanel");
    navPanel->setMinimumWidth(68);
    navPanel->setMaximumWidth(120);
    auto* navLayout = new QVBoxLayout(navPanel);
    navLayout->setContentsMargins(10, 14, 10, 14);
    navLayout->setSpacing(12);

    auto* avatar = new QToolButton(navPanel);
    avatar->setObjectName("navAvatar");
    avatar->setText(m_currentUserId.left(1).toUpper());
    avatar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    avatar->setCursor(Qt::PointingHandCursor);
    avatar->setFixedSize(42, 42);
    navLayout->addWidget(avatar, 0, Qt::AlignHCenter);

    auto* chatBtn = new QPushButton("聊", navPanel);
    chatBtn->setObjectName("navBtnActive");
    chatBtn->setCursor(Qt::PointingHandCursor);
    chatBtn->setFixedSize(40, 40);
    navLayout->addWidget(chatBtn, 0, Qt::AlignHCenter);

    auto* contactBtn = new QPushButton("友", navPanel);
    contactBtn->setObjectName("navBtn");
    contactBtn->setCursor(Qt::PointingHandCursor);
    contactBtn->setFixedSize(40, 40);
    navLayout->addWidget(contactBtn, 0, Qt::AlignHCenter);
    navLayout->addStretch();

    m_contactWidget = new ContactWidget(central);
    m_contactWidget->setMinimumWidth(240);
    m_contactWidget->setMaximumWidth(500);
    m_chatWidget = new ChatWidget(central);
    m_chatWidget->setCurrentUserId(m_currentUserId);

    splitter->addWidget(navPanel);
    splitter->addWidget(m_contactWidget);
    splitter->addWidget(m_chatWidget);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 3);
    splitter->setSizes({80, 300, 900});
    setCentralWidget(central);

    connect(avatar, &QToolButton::clicked, this, [this]() {
        QMessageBox::information(
            this,
            "个人信息",
            QString("用户ID: %1\n状态: 在线").arg(m_currentUserId)
        );
    });

    setStyleSheet(
        "QWidget#mainRoot { background: #ededed; }"
        "QFrame#navPanel { background: #2e2f33; border-right: 1px solid #25262a; }"
        "QSplitter::handle:horizontal { background: #d9d9d9; }"
        "QSplitter::handle:horizontal:hover { background: #c6c6c6; }"
        "QToolButton#navAvatar { background: #4c8cf5; color: white; border-radius: 21px; font-size: 17px; font-weight: 700; border: none; }"
        "QToolButton#navAvatar:hover { background: #3f80ec; }"
        "QPushButton#navBtn, QPushButton#navBtnActive { border-radius: 20px; font-size: 16px; font-weight: 700; border: none; color: #d5d5d5; }"
        "QPushButton#navBtn { background: transparent; }"
        "QPushButton#navBtn:hover { background: #3c3d42; }"
        "QPushButton#navBtnActive { background: #3f8cff; color: white; }"
    );
}

void MainWindow::wireSignals() {
    if (!m_protocol || !m_contactWidget || !m_chatWidget) return;
    connect(m_contactWidget, &ContactWidget::conversationSelected,
            this, &MainWindow::onConversationSelected);
    connect(m_chatWidget, &ChatWidget::messageSent,
            this, &MainWindow::sendChatMessage);

    connect(m_protocol, &ProtocolHandler::newMessageNotify,
            this, &MainWindow::onPushNewMessage);
    connect(m_protocol, &ProtocolHandler::recallNotify,
            this, &MainWindow::onPushRecall);
    connect(m_protocol, &ProtocolHandler::readReceiptNotify,
            this, &MainWindow::onPushReadReceipt);
}

void MainWindow::syncConversations() {
    if (!m_protocol) return;
    swift::zone::ChatSyncConversationsPayload req;
    req.set_last_sync_time(0);
    std::string payload;
    if (!req.SerializeToString(&payload)) return;
    m_protocol->sendRequest("chat.sync_conversations",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this](int code, const QByteArray& data) {
        if (code != 0) return;
        swift::zone::ChatSyncConversationsResponsePayload resp;
        if (!resp.ParseFromArray(data.data(), data.size())) return;

        QList<Conversation> convs;
        m_conversationMap.clear();
        for (const auto& c : resp.conversations()) {
            Conversation conv;
            conv.chatId = QString::fromStdString(c.chat_id());
            conv.chatType = c.chat_type();
            conv.peerId = QString::fromStdString(c.peer_id());
            conv.peerName = QString::fromStdString(c.peer_name());
            conv.peerAvatar = QString::fromStdString(c.peer_avatar());
            conv.unreadCount = c.unread_count();
            conv.updatedAt = c.updated_at();
            conv.lastMessage.msgId = QString::fromStdString(c.last_msg_id());
            conv.lastMessage.content = QString::fromStdString(c.last_content());
            conv.lastMessage.timestamp = c.last_timestamp();

            m_conversationMap[convKey(conv.chatId, conv.chatType)] = conv;
            convs.append(conv);
        }
        m_contactWidget->setConversations(convs);
    });
}

void MainWindow::loadHistory(const QString& chatId, int chatType) {
    if (!m_protocol) return;
    swift::zone::ChatGetHistoryPayload req;
    req.set_chat_id(chatId.toStdString());
    req.set_chat_type(chatType);
    req.set_limit(50);
    std::string payload;
    if (!req.SerializeToString(&payload)) return;

    m_protocol->sendRequest("chat.get_history",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this, chatId, chatType](int code, const QByteArray& data) {
        if (code != 0) return;
        swift::zone::ChatGetHistoryResponsePayload resp;
        if (!resp.ParseFromArray(data.data(), data.size())) return;

        QList<Message> messages;
        for (const auto& m : resp.messages()) {
            Message msg;
            msg.msgId = QString::fromStdString(m.msg_id());
            msg.fromUserId = QString::fromStdString(m.from_user_id());
            msg.toId = QString::fromStdString(m.to_id());
            msg.chatType = m.chat_type();
            msg.content = QString::fromStdString(m.content());
            msg.mediaUrl = QString::fromStdString(m.media_url());
            msg.mediaType = QString::fromStdString(m.media_type());
            msg.timestamp = m.timestamp();
            msg.isSelf = (msg.fromUserId == m_currentUserId);
            messages.append(msg);
        }

        const QString key = convKey(chatId, chatType);
        m_messageMap[key] = messages;
        if (chatId == m_currentChatId && chatType == m_currentChatType) {
            m_chatWidget->setMessages(messages);
            sendMarkRead();
        }
    });
}

void MainWindow::sendChatMessage(const QString& content) {
    if (!m_protocol || m_currentChatId.isEmpty() || content.isEmpty()) return;

    swift::zone::ChatSendMessagePayload req;
    req.set_from_user_id(m_currentUserId.toStdString());
    req.set_to_id(m_currentChatId.toStdString());
    req.set_chat_type(m_currentChatType);
    req.set_content(content.toStdString());
    req.set_media_type("text");
    req.set_client_msg_id(QString("c_%1").arg(QDateTime::currentMSecsSinceEpoch()).toStdString());

    std::string payload;
    if (!req.SerializeToString(&payload)) return;
    m_protocol->sendRequest("chat.send_message",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this, content](int code, const QByteArray& data) {
        if (code != 0) return;
        swift::zone::ChatSendMessageResponsePayload resp;
        if (!resp.ParseFromArray(data.data(), data.size())) return;
        if (!resp.success()) return;

        Message msg;
        msg.msgId = QString::fromStdString(resp.msg_id());
        msg.fromUserId = m_currentUserId;
        msg.toId = m_currentChatId;
        msg.chatType = m_currentChatType;
        msg.content = content;
        msg.mediaType = "text";
        msg.timestamp = resp.timestamp();
        msg.isSelf = true;

        const QString key = convKey(m_currentChatId, m_currentChatType);
        m_messageMap[key].append(msg);
        if (m_chatWidget->chatId() == m_currentChatId && m_chatWidget->chatType() == m_currentChatType) {
            m_chatWidget->appendMessage(msg);
        }

        auto convIt = m_conversationMap.find(key);
        if (convIt != m_conversationMap.end()) {
            convIt->lastMessage = msg;
            convIt->updatedAt = msg.timestamp;
            m_contactWidget->upsertConversation(*convIt);
        }
    });
}

void MainWindow::sendMarkRead() {
    if (!m_protocol || m_currentChatId.isEmpty()) return;
    const QString key = convKey(m_currentChatId, m_currentChatType);
    const auto msgs = m_messageMap.value(key);
    if (msgs.isEmpty()) return;

    swift::zone::ChatMarkReadPayload req;
    req.set_chat_id(m_currentChatId.toStdString());
    req.set_chat_type(m_currentChatType);
    req.set_last_msg_id(msgs.last().msgId.toStdString());
    std::string payload;
    if (!req.SerializeToString(&payload)) return;
    m_protocol->sendRequest("chat.mark_read",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            nullptr);
}

QString MainWindow::convKey(const QString& chatId, int chatType) const {
    return QString("%1#%2").arg(chatType).arg(chatId);
}

void MainWindow::onConversationSelected(const QString& chatId, int chatType) {
    m_currentChatId = chatId;
    m_currentChatType = chatType;
    m_contactWidget->setCurrentChat(chatId);
    m_chatWidget->setConversation(chatId, chatType);
    const QString key = convKey(chatId, chatType);
    if (m_readReceiptMap.contains(chatId)) {
        const QStringList parts = m_readReceiptMap.value(chatId).split(':');
        if (parts.size() == 2) {
            m_chatWidget->setReadReceipt(parts[0], parts[1]);
        }
    } else {
        m_chatWidget->clearReadReceipt();
    }
    if (m_messageMap.contains(key) && !m_messageMap.value(key).isEmpty()) {
        m_chatWidget->setMessages(m_messageMap.value(key));
        sendMarkRead();
        return;
    }
    loadHistory(chatId, chatType);
}

void MainWindow::onPushNewMessage(const QByteArray& payload) {
    swift::zone::ChatMessagePushPayload push;
    if (!push.ParseFromArray(payload.data(), payload.size())) {
        swift::gate::NewMessageNotify legacy;
        if (!legacy.ParseFromArray(payload.data(), payload.size())) return;
        Message m;
        m.msgId = QString::fromStdString(legacy.msg_id());
        m.fromUserId = QString::fromStdString(legacy.from_user_id());
        m.toId = QString::fromStdString(legacy.chat_id());
        m.chatType = legacy.chat_type();
        m.content = QString::fromStdString(legacy.content());
        m.mediaUrl = QString::fromStdString(legacy.media_url());
        m.mediaType = QString::fromStdString(legacy.media_type());
        m.timestamp = legacy.timestamp();
        m.isSelf = (m.fromUserId == m_currentUserId);
        const QString key = convKey(m.toId, m.chatType);
        m_messageMap[key].append(m);
        if (m.toId == m_currentChatId && m.chatType == m_currentChatType) {
            m_chatWidget->appendMessage(m);
            sendMarkRead();
        } else {
            m_contactWidget->increaseUnreadForChat(m.toId);
        }
        return;
    }

    Message msg;
    msg.msgId = QString::fromStdString(push.msg_id());
    msg.fromUserId = QString::fromStdString(push.from_user_id());
    msg.toId = QString::fromStdString(push.to_id());
    msg.chatType = push.chat_type();
    msg.content = QString::fromStdString(push.content());
    msg.mediaUrl = QString::fromStdString(push.media_url());
    msg.mediaType = QString::fromStdString(push.media_type());
    msg.timestamp = push.timestamp();
    msg.isSelf = (msg.fromUserId == m_currentUserId);

    const QString key = convKey(msg.toId, msg.chatType);
    m_messageMap[key].append(msg);

    Conversation conv = m_conversationMap.value(key);
    conv.chatId = msg.toId;
    conv.chatType = msg.chatType;
    if (conv.peerName.isEmpty()) conv.peerName = msg.toId;
    conv.lastMessage = msg;
    conv.updatedAt = msg.timestamp;
    if (!(msg.toId == m_currentChatId && msg.chatType == m_currentChatType)) {
        conv.unreadCount += 1;
    }
    m_conversationMap[key] = conv;
    m_contactWidget->upsertConversation(conv);

    if (msg.toId == m_currentChatId && msg.chatType == m_currentChatType) {
        m_chatWidget->appendMessage(msg);
        sendMarkRead();
    }
}

void MainWindow::onPushRecall(const QByteArray& payload) {
    swift::gate::RecallNotify notify;
    if (!notify.ParseFromArray(payload.data(), payload.size())) return;
    const QString chatId = QString::fromStdString(notify.chat_id());
    const int chatType = notify.chat_type();
    const QString key = convKey(chatId, chatType);
    const QString msgId = QString::fromStdString(notify.msg_id());
    auto msgs = m_messageMap.value(key);
    for (auto& m : msgs) {
        if (m.msgId == msgId) {
            m.status = 1;
            break;
        }
    }
    m_messageMap[key] = msgs;
    if (chatId == m_currentChatId && chatType == m_currentChatType) {
        m_chatWidget->markMessageRecalled(msgId);
    }
}

void MainWindow::onPushReadReceipt(const QByteArray& payload) {
    swift::gate::ReadReceiptNotify notify;
    if (!notify.ParseFromArray(payload.data(), payload.size())) return;
    const QString chatId = QString::fromStdString(notify.chat_id());
    const QString userId = QString::fromStdString(notify.user_id());
    const QString lastMsgId = QString::fromStdString(notify.last_read_msg_id());
    if (chatId.isEmpty() || userId.isEmpty() || lastMsgId.isEmpty()) return;

    m_readReceiptMap[chatId] = QString("%1:%2").arg(userId, lastMsgId);
    if (chatId == m_currentChatId) {
        m_chatWidget->setReadReceipt(userId, lastMsgId);
    }
}
