#include "chatwidget.h"

#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QString BuildSenderName(const Message& m) {
    if (m.isSelf) return QStringLiteral("我");
    return m.fromUserId.isEmpty() ? QStringLiteral("对方") : m.fromUserId;
}

QString BuildMessageBody(const Message& m) {
    if (m.status == 1) {
        return QStringLiteral("这条消息已撤回");
    }
    if (m.content.trimmed().isEmpty()) {
        return QStringLiteral("[空消息]");
    }
    return m.content;
}

QString BuildTimeText(qint64 timestamp) {
    if (timestamp <= 0) return QString();
    return QDateTime::fromMSecsSinceEpoch(timestamp).toString("MM-dd HH:mm");
}

}  // namespace

ChatWidget::ChatWidget(QWidget *parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(14, 12, 14, 12);
    root->setSpacing(10);

    auto* header = new QFrame(this);
    header->setObjectName("chatHeader");
    auto* headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(14, 12, 14, 12);
    headerLayout->setSpacing(2);
    m_titleLabel = new QLabel("请选择一个会话", header);
    m_titleLabel->setObjectName("chatTitle");
    m_subtitleLabel = new QLabel("可在左侧选择联系人开始聊天", header);
    m_subtitleLabel->setObjectName("chatSubtitle");
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addWidget(m_subtitleLabel);
    root->addWidget(header);

    m_messageList = new QListWidget(this);
    m_messageList->setFrameShape(QFrame::NoFrame);
    m_messageList->setSpacing(10);
    m_messageList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_messageList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_messageList->setStyleSheet(
        "QListWidget { background: transparent; border: none; outline: none; }"
        "QListWidget::item { border: none; padding: 0px; margin: 0px; }"
        "QListWidget::item:selected { background: transparent; }"
    );
    root->addWidget(m_messageList, 1);

    m_readReceiptLabel = new QLabel(this);
    m_readReceiptLabel->setStyleSheet("color: #7d8796; font-size: 12px;");
    m_readReceiptLabel->setText("");
    root->addWidget(m_readReceiptLabel);

    auto* bottom = new QHBoxLayout();
    bottom->setSpacing(8);
    m_input = new QLineEdit(this);
    m_input->setPlaceholderText("输入消息...");
    m_input->setMinimumHeight(40);
    m_input->setStyleSheet(
        "QLineEdit { background: #ffffff; border: 1px solid #d6dce8; border-radius: 10px; padding: 0 12px; font-size: 14px; }"
        "QLineEdit:focus { border-color: #6699ff; }"
    );
    m_sendBtn = new QPushButton("发送", this);
    m_sendBtn->setMinimumHeight(40);
    m_sendBtn->setMinimumWidth(88);
    m_sendBtn->setCursor(Qt::PointingHandCursor);
    m_sendBtn->setStyleSheet(
        "QPushButton { background: #3f8cff; color: white; border: none; border-radius: 10px; font-size: 14px; font-weight: 600; padding: 0 16px; }"
        "QPushButton:hover { background: #2d7df8; }"
        "QPushButton:pressed { background: #1f6ce0; }"
        "QPushButton:disabled { background: #adc8f4; color: #eef5ff; }"
    );
    bottom->addWidget(m_input, 1);
    bottom->addWidget(m_sendBtn);
    root->addLayout(bottom);

    setStyleSheet(
        "QFrame#chatHeader { background: #ffffff; border: 1px solid #e3e9f4; border-radius: 12px; }"
        "QLabel#chatTitle { color: #202734; font-size: 15px; font-weight: 600; }"
        "QLabel#chatSubtitle { color: #7f8796; font-size: 12px; }"
    );

    connect(m_sendBtn, &QPushButton::clicked, this, &ChatWidget::onSendClicked);
    connect(m_input, &QLineEdit::returnPressed, this, &ChatWidget::onSendClicked);
}

ChatWidget::~ChatWidget() = default;

void ChatWidget::setConversation(const QString& chatId, int chatType) {
    m_chatId = chatId;
    m_chatType = chatType;
    m_messages.clear();
    updateHeader();
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
    addMessageItem(message);
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
        addMessageItem(msg);
    }
    m_messageList->scrollToBottom();
}

void ChatWidget::addMessageItem(const Message& message) {
    if (!m_messageList) return;
    auto* item = new QListWidgetItem(m_messageList);
    item->setData(Qt::UserRole, message.msgId);
    auto* row = buildMessageItemWidget(message);
    item->setSizeHint(row->sizeHint());
    m_messageList->setItemWidget(item, row);
    m_messageList->scrollToBottom();
}

QWidget* ChatWidget::buildMessageItemWidget(const Message& message) const {
    auto* container = new QWidget();
    auto* row = new QHBoxLayout(container);
    row->setContentsMargins(2, 0, 2, 0);
    row->setSpacing(10);

    auto* bubble = new QFrame(container);
    bubble->setObjectName("msgBubble");
    bubble->setProperty("self", message.isSelf);
    bubble->setProperty("recalled", message.status == 1);
    bubble->setMaximumWidth(560);

    auto* bubbleLayout = new QVBoxLayout(bubble);
    bubbleLayout->setContentsMargins(12, 8, 12, 8);
    bubbleLayout->setSpacing(4);

    auto* sender = new QLabel(BuildSenderName(message), bubble);
    sender->setObjectName("msgSender");
    auto* content = new QLabel(BuildMessageBody(message), bubble);
    content->setObjectName("msgContent");
    content->setWordWrap(true);
    auto* meta = new QLabel(BuildTimeText(message.timestamp), bubble);
    meta->setObjectName("msgMeta");

    bubbleLayout->addWidget(sender);
    bubbleLayout->addWidget(content);
    bubbleLayout->addWidget(meta, 0, message.isSelf ? Qt::AlignRight : Qt::AlignLeft);

    bubble->setStyleSheet(
        "QFrame#msgBubble { border-radius: 12px; border: 1px solid #dfe6f3; background: #ffffff; }"
        "QFrame#msgBubble[self='true'] { background: #dfeeff; border-color: #bed8ff; }"
        "QFrame#msgBubble[recalled='true'] { background: #f6f7f9; border-color: #e6e8ed; }"
        "QLabel#msgSender { color: #5a677c; font-size: 11px; font-weight: 600; }"
        "QLabel#msgContent { color: #202734; font-size: 14px; line-height: 1.4; }"
        "QFrame#msgBubble[recalled='true'] QLabel#msgContent { color: #9aa2b2; font-style: italic; }"
        "QLabel#msgMeta { color: #8b94a4; font-size: 11px; }"
    );

    if (message.isSelf) {
        row->addStretch();
        row->addWidget(bubble, 0, Qt::AlignRight);
    } else {
        row->addWidget(bubble, 0, Qt::AlignLeft);
        row->addStretch();
    }
    return container;
}

void ChatWidget::updateHeader() {
    if (!m_titleLabel || !m_subtitleLabel) return;
    if (m_chatId.isEmpty()) {
        m_titleLabel->setText("请选择一个会话");
        m_subtitleLabel->setText("可在左侧选择联系人开始聊天");
        return;
    }
    m_titleLabel->setText(buildChatTitle());
    m_subtitleLabel->setText(m_chatType == 2 ? "群聊会话" : "私聊会话");
}

QString ChatWidget::buildChatTitle() const {
    if (m_chatId.isEmpty()) return QStringLiteral("未选择会话");
    return QString("与 %1 聊天").arg(m_chatId);
}
