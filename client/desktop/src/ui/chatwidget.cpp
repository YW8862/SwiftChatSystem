#include "chatwidget.h"

#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QScrollBar>
#include <QSet>
#include <QSplitter>
#include <QTextEdit>
#include <QVBoxLayout>
#include <algorithm>

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

bool IsFileMessage(const Message& m) {
    return m.mediaType.compare("file", Qt::CaseInsensitive) == 0;
}

QString BuildFileName(const Message& m) {
    if (!m.content.trimmed().isEmpty()) {
        return m.content.trimmed();
    }
    if (!m.mediaUrl.trimmed().isEmpty()) {
        QFileInfo info(m.mediaUrl.trimmed());
        if (!info.fileName().isEmpty()) {
            return info.fileName();
        }
    }
    return QStringLiteral("未命名文件");
}

QString BuildTimeText(qint64 timestamp) {
    if (timestamp <= 0) return QString();
    return QDateTime::fromMSecsSinceEpoch(timestamp).toString("MM-dd HH:mm");
}

QString BuildMetaText(const Message& m) {
    QString base = BuildTimeText(m.timestamp);
    if (m.isSelf) {
        if (m.sending) {
            return base.isEmpty() ? QStringLiteral("发送中...") : QString("%1 · 发送中...").arg(base);
        }
        if (m.sendFailed) {
            return base.isEmpty() ? QStringLiteral("发送失败 · 双击重发")
                                  : QString("%1 · 发送失败 · 双击重发").arg(base);
        }
        return base.isEmpty() ? QStringLiteral("已发送") : QString("%1 · 已发送").arg(base);
    }
    return base;
}

QString BuildSeparatorText(qint64 timestamp) {
    if (timestamp <= 0) return QStringLiteral("刚刚");
    const QDateTime dt = QDateTime::fromMSecsSinceEpoch(timestamp);
    const QDate date = dt.date();
    const QDate today = QDate::currentDate();
    if (date == today) {
        return dt.toString("HH:mm");
    }
    if (date == today.addDays(-1)) {
        return QString("昨天 %1").arg(dt.toString("HH:mm"));
    }
    if (date == today.addDays(-2)) {
        return QString("前天 %1").arg(dt.toString("HH:mm"));
    }
    return dt.toString("yyyy/MM/dd HH:mm");
}

bool ShouldInsertSeparator(qint64 previousTs, qint64 currentTs) {
    if (currentTs <= 0) return false;
    if (previousTs <= 0) return true;
    const QDateTime prev = QDateTime::fromMSecsSinceEpoch(previousTs);
    const QDateTime curr = QDateTime::fromMSecsSinceEpoch(currentTs);
    // 微信风格：同一分钟内连续消息仅显示一次时间条。
    return prev.toString("yyyyMMddHHmm") != curr.toString("yyyyMMddHHmm");
}

}  // namespace

ChatWidget::ChatWidget(QWidget *parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_headerFrame = new QFrame(this);
    m_headerFrame->setObjectName("chatHeader");
    auto* headerLayout = new QHBoxLayout(m_headerFrame);
    headerLayout->setContentsMargins(18, 12, 18, 12);
    headerLayout->setSpacing(8);
    auto* titleWrap = new QVBoxLayout();
    titleWrap->setSpacing(2);
    titleWrap->setContentsMargins(0, 0, 0, 0);
    m_titleLabel = new QLabel("", m_headerFrame);
    m_titleLabel->setObjectName("chatTitle");
    m_subtitleLabel = new QLabel("", m_headerFrame);
    m_subtitleLabel->setObjectName("chatSubtitle");
    titleWrap->addWidget(m_titleLabel);
    titleWrap->addWidget(m_subtitleLabel);
    headerLayout->addLayout(titleWrap);
    headerLayout->addStretch();
    auto* moreBtn = new QPushButton("...", m_headerFrame);
    moreBtn->setObjectName("chatMoreBtn");
    moreBtn->setFixedSize(30, 30);
    headerLayout->addWidget(moreBtn);
    root->addWidget(m_headerFrame);

    m_bodyFrame = new QFrame(this);
    m_bodyFrame->setObjectName("chatBody");
    auto* bodyLayout = new QVBoxLayout(m_bodyFrame);
    bodyLayout->setContentsMargins(16, 12, 16, 10);
    bodyLayout->setSpacing(0);

    m_verticalSplitter = new QSplitter(Qt::Vertical, m_bodyFrame);
    m_verticalSplitter->setChildrenCollapsible(false);
    m_verticalSplitter->setHandleWidth(6);

    auto* messagePanel = new QFrame(m_verticalSplitter);
    auto* messageLayout = new QVBoxLayout(messagePanel);
    messageLayout->setContentsMargins(0, 0, 0, 8);
    messageLayout->setSpacing(8);

    m_messageList = new QListWidget(messagePanel);
    m_messageList->setFrameShape(QFrame::NoFrame);
    m_messageList->setSpacing(12);
    m_messageList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_messageList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_messageList->setStyleSheet(
        "QListWidget { background: #f5f5f5; border: none; outline: none; }"
        "QListWidget::item { border: none; padding: 0px; margin: 0px; }"
        "QListWidget::item:selected { background: transparent; }"
    );
    messageLayout->addWidget(m_messageList, 1);

    m_readReceiptLabel = new QLabel(messagePanel);
    m_readReceiptLabel->setStyleSheet("color: #8d8d8d; font-size: 12px; padding-left: 4px;");
    m_readReceiptLabel->setText("");
    messageLayout->addWidget(m_readReceiptLabel);

    auto* inputCard = new QFrame(m_verticalSplitter);
    inputCard->setObjectName("inputCard");
    auto* inputRoot = new QVBoxLayout(inputCard);
    inputRoot->setContentsMargins(12, 8, 12, 10);
    inputRoot->setSpacing(8);

    auto* inputTools = new QHBoxLayout();
    inputTools->setContentsMargins(0, 0, 0, 0);
    inputTools->setSpacing(6);
    auto* emojiBtn = new QPushButton("😊", inputCard);
    emojiBtn->setObjectName("inputToolBtn");
    emojiBtn->setFixedSize(30, 30);
    auto* fileBtn = new QPushButton("+", inputCard);
    fileBtn->setObjectName("inputToolBtn");
    fileBtn->setFixedSize(30, 30);
    inputTools->addWidget(emojiBtn);
    inputTools->addWidget(fileBtn);
    inputTools->addStretch();
    inputRoot->addLayout(inputTools);

    m_input = new QTextEdit(inputCard);
    m_input->setPlaceholderText("输入消息（Enter 发送，Shift+Enter 换行）");
    m_input->setMinimumHeight(88);
    m_input->setMaximumHeight(150);
    m_input->setAcceptRichText(false);
    m_input->setStyleSheet(
        "QTextEdit { background: #ffffff; border: 1px solid #e0e0e0; border-radius: 8px; padding: 8px 10px; font-size: 14px; }"
        "QTextEdit:focus { border-color: #84b9ff; }"
    );
    inputRoot->addWidget(m_input);

    auto* actionRow = new QHBoxLayout();
    actionRow->setSpacing(8);
    auto* hint = new QLabel("按需输入消息并点击发送", inputCard);
    hint->setObjectName("sendHint");
    actionRow->addWidget(hint);
    actionRow->addStretch();
    m_sendBtn = new QPushButton("发送", this);
    m_sendBtn->setMinimumHeight(34);
    m_sendBtn->setMinimumWidth(92);
    m_sendBtn->setCursor(Qt::PointingHandCursor);
    m_sendBtn->setStyleSheet(
        "QPushButton { background: #07c160; color: white; border: none; border-radius: 6px; font-size: 13px; font-weight: 600; padding: 0 16px; }"
        "QPushButton:hover { background: #06ad56; }"
        "QPushButton:pressed { background: #05974b; }"
        "QPushButton:disabled { background: #9adbb9; color: #edf9f3; }"
    );
    actionRow->addWidget(m_sendBtn);
    inputRoot->addLayout(actionRow);

    bodyLayout->addWidget(m_verticalSplitter);
    root->addWidget(m_bodyFrame, 1);
    m_verticalSplitter->setStretchFactor(0, 5);
    m_verticalSplitter->setStretchFactor(1, 2);

    setStyleSheet(
        "ChatWidget { background: #f5f5f5; }"
        "QFrame#chatHeader { background: #f5f5f5; border-bottom: 1px solid #e0e0e0; }"
        "QLabel#chatTitle { color: #1f1f1f; font-size: 15px; font-weight: 600; }"
        "QLabel#chatSubtitle { color: #8c8c8c; font-size: 12px; }"
        "QPushButton#chatMoreBtn { border: none; background: transparent; color: #8d8d8d; font-size: 18px; }"
        "QPushButton#chatMoreBtn:hover { background: #ebebeb; border-radius: 8px; }"
        "QFrame#chatBody { background: #f5f5f5; }"
        "QSplitter::handle:vertical { background: #e8e8e8; }"
        "QSplitter::handle:vertical:hover { background: #d6d6d6; }"
        "QFrame#inputCard { background: #ffffff; border-top: 1px solid #dfdfdf; }"
        "QPushButton#inputToolBtn { background: transparent; border: none; color: #707070; font-size: 16px; }"
        "QPushButton#inputToolBtn:hover { background: #f2f2f2; border-radius: 6px; }"
        "QLabel#sendHint { color: #9b9b9b; font-size: 12px; }"
    );

    connect(m_sendBtn, &QPushButton::clicked, this, &ChatWidget::onSendClicked);
    connect(fileBtn, &QPushButton::clicked, this, &ChatWidget::onFileClicked);
    connect(m_messageList->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int value) {
        if (!m_messageList || m_chatId.isEmpty()) return;
        if (value >= m_messageList->verticalScrollBar()->maximum()) {
            emit messageListReachedBottom();
        }
    });
    connect(m_messageList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        if (!item) return;
        const QString msgId = item->data(Qt::UserRole).toString();
        if (msgId.isEmpty()) return;
        for (const auto& msg : m_messages) {
            if (msg.msgId == msgId && msg.sendFailed) {
                emit retryMessageRequested(msgId);
                return;
            }
            if (msg.msgId == msgId && msg.status != 1 && IsFileMessage(msg)) {
                emit fileMessageOpenRequested(msgId);
                return;
            }
            if (msg.msgId == msgId && msg.isSelf && !msg.sending && !msg.sendFailed && msg.status != 1) {
                emit recallMessageRequested(msgId);
                return;
            }
        }
    });
    updateConversationVisibility();
}

ChatWidget::~ChatWidget() = default;

void ChatWidget::setConversation(const QString& chatId, int chatType) {
    m_chatId = chatId;
    m_chatType = chatType;
    m_messages.clear();
    updateHeader();
    updateConversationVisibility();
    refreshMessageList(true);
    clearReadReceipt();
}

void ChatWidget::setCurrentUserId(const QString& userId) {
    m_currentUserId = userId;
}

void ChatWidget::setMessages(const QList<Message>& messages) {
    m_messages = normalizeMessages(messages);
    updateConversationVisibility();
    refreshMessageList(true);
}

void ChatWidget::prependHistoryMessages(const QList<Message>& messages) {
    if (messages.isEmpty() || !m_messageList) return;
    const int oldValue = m_messageList->verticalScrollBar()->value();
    const int oldMax = m_messageList->verticalScrollBar()->maximum();

    QList<Message> merged = messages;
    merged.append(m_messages);
    m_messages = normalizeMessages(merged);
    refreshMessageList(false, oldValue, oldMax);
}

void ChatWidget::appendMessage(const Message& message) {
    if (!message.msgId.isEmpty()) {
        for (const auto& m : m_messages) {
            if (m.msgId == message.msgId) return;
        }
    }
    m_messages.append(message);
    updateConversationVisibility();
    refreshMessageList(true);
}

void ChatWidget::markMessageRecalled(const QString& msgId) {
    for (auto& msg : m_messages) {
        if (msg.msgId == msgId) {
            msg.status = 1;
            break;
        }
    }
    refreshMessageList(true);
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
    const QString content = m_input->toPlainText().trimmed();
    if (content.isEmpty()) return;
    emit messageSent(content);
    m_input->setPlainText("");
}

void ChatWidget::onFileClicked() {
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("选择文件"),
        QString(),
        QStringLiteral("所有文件 (*.*)")
    );
    if (filePath.isEmpty()) return;
    emit fileSelected(filePath);
}

QList<Message> ChatWidget::normalizeMessages(const QList<Message>& messages) const {
    QList<Message> normalized = messages;
    std::sort(normalized.begin(), normalized.end(),
              [](const Message& a, const Message& b) {
                  if (a.timestamp == b.timestamp) {
                      return a.msgId < b.msgId;
                  }
                  return a.timestamp < b.timestamp;
              });

    QList<Message> deduped;
    deduped.reserve(normalized.size());
    QSet<QString> seenIds;
    for (const auto& msg : normalized) {
        if (!msg.msgId.isEmpty()) {
            if (seenIds.contains(msg.msgId)) continue;
            seenIds.insert(msg.msgId);
        }
        deduped.append(msg);
    }
    return deduped;
}

void ChatWidget::refreshMessageList(bool scrollToBottom, int preserveScrollValue, int preserveScrollMax) {
    if (!m_messageList) return;
    m_messageList->clear();
    qint64 previousTimestamp = 0;
    for (const auto& msg : m_messages) {
        if (ShouldInsertSeparator(previousTimestamp, msg.timestamp)) {
            addTimeSeparatorItem(BuildSeparatorText(msg.timestamp));
        }
        addMessageItem(msg);
        previousTimestamp = msg.timestamp;
    }
    if (scrollToBottom) {
        m_messageList->scrollToBottom();
        return;
    }
    if (preserveScrollValue >= 0 && preserveScrollMax >= 0) {
        QScrollBar* bar = m_messageList->verticalScrollBar();
        const int newMax = bar->maximum();
        bar->setValue(preserveScrollValue + (newMax - preserveScrollMax));
    }
}

void ChatWidget::addTimeSeparatorItem(const QString& text) {
    if (!m_messageList) return;
    auto* item = new QListWidgetItem(m_messageList);
    item->setFlags(Qt::NoItemFlags);

    auto* container = new QWidget();
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 4, 0, 2);
    auto* label = new QLabel(text, container);
    label->setObjectName("timeSeparator");
    label->setAlignment(Qt::AlignCenter);
    layout->addStretch();
    layout->addWidget(label);
    layout->addStretch();
    container->setStyleSheet(
        "QLabel#timeSeparator { background: #dadada; color: #666666; border-radius: 10px; font-size: 11px; padding: 3px 10px; }"
    );
    item->setSizeHint(container->sizeHint());
    m_messageList->setItemWidget(item, container);
}

void ChatWidget::addMessageItem(const Message& message) {
    if (!m_messageList) return;
    auto* item = new QListWidgetItem(m_messageList);
    item->setData(Qt::UserRole, message.msgId);
    auto* row = buildMessageItemWidget(message);
    item->setSizeHint(row->sizeHint());
    m_messageList->setItemWidget(item, row);
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
    bubble->setProperty("sending", message.sending);
    bubble->setProperty("sendFailed", message.sendFailed);
    bubble->setMaximumWidth(520);

    auto* bubbleLayout = new QVBoxLayout(bubble);
    bubbleLayout->setContentsMargins(12, 8, 12, 8);
    bubbleLayout->setSpacing(4);

    auto* sender = new QLabel(BuildSenderName(message), bubble);
    sender->setObjectName("msgSender");
    auto* meta = new QLabel(BuildMetaText(message), bubble);
    meta->setObjectName("msgMeta");

    bubbleLayout->addWidget(sender);
    if (message.status != 1 && IsFileMessage(message)) {
        auto* fileCard = new QFrame(bubble);
        fileCard->setObjectName("fileCard");
        auto* fileLayout = new QHBoxLayout(fileCard);
        fileLayout->setContentsMargins(10, 8, 10, 8);
        fileLayout->setSpacing(10);

        auto* icon = new QLabel("FILE", fileCard);
        icon->setObjectName("fileIcon");
        icon->setAlignment(Qt::AlignCenter);
        icon->setFixedSize(40, 40);

        auto* textWrap = new QVBoxLayout();
        textWrap->setContentsMargins(0, 0, 0, 0);
        textWrap->setSpacing(2);
        auto* fileName = new QLabel(BuildFileName(message), fileCard);
        fileName->setObjectName("fileName");
        fileName->setWordWrap(true);
        auto* fileHint = new QLabel("文件消息", fileCard);
        fileHint->setObjectName("fileHint");
        textWrap->addWidget(fileName);
        textWrap->addWidget(fileHint);

        fileLayout->addWidget(icon);
        fileLayout->addLayout(textWrap, 1);
        bubbleLayout->addWidget(fileCard);
    } else {
        auto* content = new QLabel(BuildMessageBody(message), bubble);
        content->setObjectName("msgContent");
        content->setWordWrap(true);
        bubbleLayout->addWidget(content);
    }
    bubbleLayout->addWidget(meta, 0, message.isSelf ? Qt::AlignRight : Qt::AlignLeft);

    bubble->setStyleSheet(
        "QFrame#msgBubble { border-radius: 10px; border: 1px solid #e4e4e4; background: #ffffff; }"
        "QFrame#msgBubble[self='true'] { background: #d9fdd3; border-color: #bae6b0; }"
        "QFrame#msgBubble[sending='true'] { background: #edf5ff; border-color: #bfd6ff; }"
        "QFrame#msgBubble[sendFailed='true'] { background: #fff1f0; border-color: #ffb3ad; }"
        "QFrame#msgBubble[recalled='true'] { background: #f6f7f9; border-color: #e6e8ed; }"
        "QLabel#msgSender { color: #7a7a7a; font-size: 11px; font-weight: 600; }"
        "QLabel#msgContent { color: #222222; font-size: 14px; line-height: 1.4; }"
        "QFrame#msgBubble[recalled='true'] QLabel#msgContent { color: #9aa2b2; font-style: italic; }"
        "QLabel#msgMeta { color: #9b9b9b; font-size: 11px; }"
        "QFrame#msgBubble[sendFailed='true'] QLabel#msgMeta { color: #d34c4c; font-weight: 600; }"
        "QFrame#fileCard { background: rgba(255, 255, 255, 0.72); border: 1px solid #d7dde9; border-radius: 8px; }"
        "QFrame#msgBubble[self='true'] QFrame#fileCard { background: rgba(255, 255, 255, 0.78); border-color: #c5d9ff; }"
        "QLabel#fileIcon { color: #2f6ff4; background: #eaf1ff; border-radius: 6px; font-size: 10px; font-weight: 700; }"
        "QLabel#fileName { color: #1f2a3d; font-size: 13px; font-weight: 600; }"
        "QLabel#fileHint { color: #7d8798; font-size: 11px; }"
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
        m_titleLabel->clear();
        m_subtitleLabel->clear();
        return;
    }
    m_titleLabel->setText(buildChatTitle());
    m_subtitleLabel->setText(m_chatType == 2 ? "群聊会话" : "私聊会话");
}

void ChatWidget::updateConversationVisibility() {
    const bool hasConversation = !m_chatId.isEmpty();
    if (m_headerFrame) m_headerFrame->setVisible(hasConversation);
    if (m_bodyFrame) m_bodyFrame->setVisible(hasConversation);
}

QString ChatWidget::buildChatTitle() const {
    if (m_chatId.isEmpty()) return QStringLiteral("未选择会话");
    return QString("与 %1 聊天").arg(m_chatId);
}
