#include "contactwidget.h"

#include <QDateTime>
#include <QFrame>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>

namespace {

QString BuildPreview(const Conversation& c) {
    if (c.lastMessage.status == 1) {
        return QStringLiteral("[消息已撤回]");
    }
    const QString text = c.lastMessage.content.trimmed();
    return text.isEmpty() ? QStringLiteral("暂无消息，开始聊天吧") : text;
}

QString BuildTimeText(qint64 timestamp) {
    if (timestamp <= 0) return QString();
    const QDateTime dt = QDateTime::fromMSecsSinceEpoch(timestamp);
    if (dt.date() == QDate::currentDate()) {
        return dt.toString("HH:mm");
    }
    return dt.toString("MM-dd");
}

}  // namespace

ContactWidget::ContactWidget(QWidget *parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    auto* title = new QLabel("会话", this);
    title->setObjectName("conversationTitle");
    layout->addWidget(title);

    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setFrameShape(QFrame::NoFrame);
    m_listWidget->setSpacing(8);
    m_listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listWidget->setStyleSheet(
        "QListWidget { background: transparent; border: none; outline: none; }"
        "QListWidget::item { border: none; padding: 0px; margin: 0px; }"
        "QListWidget::item:selected { background: transparent; }"
        "QListWidget::item:hover { background: transparent; }"
    );
    layout->addWidget(m_listWidget);

    setStyleSheet(
        "ContactWidget { background: #f7f9fc; border-right: 1px solid #e4eaf4; }"
        "QLabel#conversationTitle { color: #1f2430; font-size: 18px; font-weight: 700; padding: 4px 6px; }"
    );

    connect(m_listWidget, &QListWidget::itemClicked, this, &ContactWidget::onItemClicked);
}

ContactWidget::~ContactWidget() = default;

void ContactWidget::setConversations(const QList<Conversation>& conversations) {
    m_conversations = conversations;
    refreshList();
}

void ContactWidget::upsertConversation(const Conversation& conversation) {
    for (int i = 0; i < m_conversations.size(); ++i) {
        if (m_conversations[i].chatId == conversation.chatId &&
            m_conversations[i].chatType == conversation.chatType) {
            m_conversations[i] = conversation;
            refreshList();
            return;
        }
    }
    m_conversations.prepend(conversation);
    refreshList();
}

void ContactWidget::increaseUnreadForChat(const QString& chatId) {
    for (auto& c : m_conversations) {
        if (c.chatId == chatId) {
            c.unreadCount += 1;
            refreshList();
            return;
        }
    }
}

void ContactWidget::setCurrentChat(const QString& chatId) {
    m_currentChatId = chatId;
    for (auto& c : m_conversations) {
        if (c.chatId == chatId) c.unreadCount = 0;
    }
    refreshList();
}

void ContactWidget::refreshList() {
    if (!m_listWidget) return;
    m_listWidget->clear();
    for (const auto& c : m_conversations) {
        auto* item = new QListWidgetItem(m_listWidget);
        item->setData(Qt::UserRole, c.chatId);
        item->setData(Qt::UserRole + 1, c.chatType);
        auto* rowWidget = buildConversationItemWidget(c);
        item->setSizeHint(rowWidget->sizeHint());
        m_listWidget->setItemWidget(item, rowWidget);
        if (c.chatId == m_currentChatId) {
            item->setSelected(true);
        }
    }
}

QWidget* ContactWidget::buildConversationItemWidget(const Conversation& conversation) const {
    auto* container = new QFrame();
    container->setObjectName("conversationCard");
    container->setProperty("active", conversation.chatId == m_currentChatId);

    auto* root = new QVBoxLayout(container);
    root->setContentsMargins(12, 10, 12, 10);
    root->setSpacing(6);

    auto* top = new QHBoxLayout();
    top->setSpacing(8);
    auto* name = new QLabel(conversation.peerName.isEmpty() ? conversation.chatId : conversation.peerName, container);
    name->setObjectName("conversationName");
    name->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto* time = new QLabel(BuildTimeText(conversation.updatedAt), container);
    time->setObjectName("conversationTime");
    top->addWidget(name);
    top->addWidget(time, 0, Qt::AlignRight);
    root->addLayout(top);

    auto* bottom = new QHBoxLayout();
    bottom->setSpacing(8);
    auto* preview = new QLabel(BuildPreview(conversation), container);
    preview->setObjectName("conversationPreview");
    preview->setWordWrap(false);
    preview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    preview->setTextInteractionFlags(Qt::NoTextInteraction);
    bottom->addWidget(preview);

    if (conversation.unreadCount > 0) {
        auto* unread = new QLabel(QString::number(conversation.unreadCount > 99 ? 99 : conversation.unreadCount), container);
        unread->setObjectName("unreadBadge");
        unread->setAlignment(Qt::AlignCenter);
        unread->setMinimumWidth(22);
        unread->setMinimumHeight(22);
        bottom->addWidget(unread, 0, Qt::AlignRight);
    }
    root->addLayout(bottom);

    container->setStyleSheet(
        "QFrame#conversationCard { background: #ffffff; border-radius: 12px; border: 1px solid #e7ecf4; }"
        "QFrame#conversationCard[active='true'] { background: #edf4ff; border: 1px solid #8db6ff; }"
        "QFrame#conversationCard:hover { border: 1px solid #b5c9eb; }"
        "QLabel#conversationName { color: #1f2430; font-size: 14px; font-weight: 600; }"
        "QLabel#conversationTime { color: #8c95a5; font-size: 12px; }"
        "QLabel#conversationPreview { color: #5a6475; font-size: 12px; }"
        "QLabel#unreadBadge { background: #ff5b5b; color: white; border-radius: 11px; font-size: 11px; font-weight: 700; padding: 0 6px; }"
    );
    return container;
}

void ContactWidget::onItemClicked(QListWidgetItem* item) {
    if (!item) return;
    const QString chatId = item->data(Qt::UserRole).toString();
    const int chatType = item->data(Qt::UserRole + 1).toInt();
    m_currentChatId = chatId;
    emit conversationSelected(chatId, chatType);
}
