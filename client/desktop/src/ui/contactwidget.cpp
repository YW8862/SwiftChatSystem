#include "contactwidget.h"

#include <QListWidget>
#include <QVBoxLayout>

namespace {

QString BuildItemText(const Conversation& c) {
    QString title = c.peerName.isEmpty() ? c.chatId : c.peerName;
    QString last = c.lastMessage.content;
    if (last.isEmpty()) last = "(无消息)";
    if (c.unreadCount > 0) {
        return QString("%1 [%2]\n%3").arg(title).arg(c.unreadCount).arg(last);
    }
    return QString("%1\n%2").arg(title, last);
}

}  // namespace

ContactWidget::ContactWidget(QWidget *parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_listWidget);

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
        auto* item = new QListWidgetItem(BuildItemText(c), m_listWidget);
        item->setData(Qt::UserRole, c.chatId);
        item->setData(Qt::UserRole + 1, c.chatType);
        if (c.chatId == m_currentChatId) {
            item->setSelected(true);
        }
    }
}

void ContactWidget::onItemClicked(QListWidgetItem* item) {
    if (!item) return;
    const QString chatId = item->data(Qt::UserRole).toString();
    const int chatType = item->data(Qt::UserRole + 1).toInt();
    emit conversationSelected(chatId, chatType);
}
