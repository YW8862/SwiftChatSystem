#include "chatwidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QString BuildMessageText(const Message& m) {
    const QString who = m.isSelf ? "我" : (m.fromUserId.isEmpty() ? "对方" : m.fromUserId);
    if (m.status == 1) {
        return QString("[%1] (已撤回)").arg(who);
    }
    return QString("[%1] %2").arg(who, m.content);
}

}  // namespace

ChatWidget::ChatWidget(QWidget *parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    m_messageList = new QListWidget(this);
    root->addWidget(m_messageList, 1);

    m_readReceiptLabel = new QLabel(this);
    m_readReceiptLabel->setStyleSheet("color: #888; font-size: 12px;");
    m_readReceiptLabel->setText("");
    root->addWidget(m_readReceiptLabel);

    auto* bottom = new QHBoxLayout();
    m_input = new QLineEdit(this);
    m_input->setPlaceholderText("输入消息...");
    m_sendBtn = new QPushButton("发送", this);
    bottom->addWidget(m_input, 1);
    bottom->addWidget(m_sendBtn);
    root->addLayout(bottom);

    connect(m_sendBtn, &QPushButton::clicked, this, &ChatWidget::onSendClicked);
}

ChatWidget::~ChatWidget() = default;

void ChatWidget::setConversation(const QString& chatId, int chatType) {
    m_chatId = chatId;
    m_chatType = chatType;
    m_messages.clear();
    refreshMessageList();
    clearReadReceipt();
}

void ChatWidget::setCurrentUserId(const QString& userId) {
    m_currentUserId = userId;
}

void ChatWidget::setMessages(const QList<Message>& messages) {
    m_messages = messages;
    refreshMessageList();
}

void ChatWidget::appendMessage(const Message& message) {
    m_messages.append(message);
    if (!m_messageList) return;
    auto* item = new QListWidgetItem(BuildMessageText(message), m_messageList);
    item->setData(Qt::UserRole, message.msgId);
    m_messageList->scrollToBottom();
}

void ChatWidget::markMessageRecalled(const QString& msgId) {
    for (auto& msg : m_messages) {
        if (msg.msgId == msgId) {
            msg.status = 1;
            break;
        }
    }
    refreshMessageList();
}

void ChatWidget::setReadReceipt(const QString& userId, const QString& lastReadMsgId) {
    if (!m_readReceiptLabel) return;
    m_readReceiptLabel->setText(QString("%1 已读到消息 %2").arg(userId, lastReadMsgId));
}

void ChatWidget::clearReadReceipt() {
    if (!m_readReceiptLabel) return;
    m_readReceiptLabel->clear();
}

void ChatWidget::onSendClicked() {
    if (m_chatId.isEmpty() || !m_input) return;
    const QString content = m_input->text().trimmed();
    if (content.isEmpty()) return;
    emit messageSent(content);
    m_input->clear();
}

void ChatWidget::onFileClicked() {
    // TODO: 选择文件
}

void ChatWidget::refreshMessageList() {
    if (!m_messageList) return;
    m_messageList->clear();
    for (const auto& msg : m_messages) {
        auto* item = new QListWidgetItem(BuildMessageText(msg), m_messageList);
        item->setData(Qt::UserRole, msg.msgId);
    }
    m_messageList->scrollToBottom();
}
