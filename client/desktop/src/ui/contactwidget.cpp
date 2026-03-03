#include "contactwidget.h"

#include <QDateTime>
#include <QFrame>
#include <QLineEdit>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPixmap>
#include <algorithm>
#include "utils/image_utils.h"

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
    layout->setContentsMargins(12, 10, 12, 8);
    layout->setSpacing(6);

    auto* searchWrap = new QFrame(this);
    searchWrap->setObjectName("searchWrap");
    auto* searchLayout = new QHBoxLayout(searchWrap);
    searchLayout->setContentsMargins(8, 4, 8, 4);
    m_searchEdit = new QLineEdit(searchWrap);
    m_searchEdit->setPlaceholderText("搜索");
    m_searchEdit->setClearButtonEnabled(true);
    searchLayout->addWidget(m_searchEdit);
    layout->addWidget(searchWrap);

    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setFrameShape(QFrame::NoFrame);
    m_listWidget->setSpacing(2);
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
        "ContactWidget { background: #f5f5f5; border-right: 1px solid #dfdfdf; }"
        "QFrame#searchWrap { background: #ffffff; border: 1px solid #dedede; border-radius: 8px; }"
        "QLineEdit { border: none; background: transparent; color: #303133; font-size: 13px; padding: 4px 2px; }"
    );

    connect(m_listWidget, &QListWidget::itemClicked, this, &ContactWidget::onItemClicked);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &ContactWidget::onSearchTextChanged);
}

ContactWidget::~ContactWidget() = default;

void ContactWidget::setConversations(const QList<Conversation>& conversations) {
    m_conversations = conversations;
    applyFilter(m_searchEdit ? m_searchEdit->text() : QString());
    refreshList();
}

void ContactWidget::upsertConversation(const Conversation& conversation) {
    for (int i = 0; i < m_conversations.size(); ++i) {
        if (m_conversations[i].chatId == conversation.chatId &&
            m_conversations[i].chatType == conversation.chatType) {
            m_conversations[i] = conversation;
            applyFilter(m_searchEdit ? m_searchEdit->text() : QString());
            refreshList();
            return;
        }
    }
    m_conversations.prepend(conversation);
    applyFilter(m_searchEdit ? m_searchEdit->text() : QString());
    refreshList();
}

void ContactWidget::increaseUnreadForChat(const QString& chatId) {
    for (auto& c : m_conversations) {
        if (c.chatId == chatId) {
            c.unreadCount += 1;
            applyFilter(m_searchEdit ? m_searchEdit->text() : QString());
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
    applyFilter(m_searchEdit ? m_searchEdit->text() : QString());
    refreshList();
}

void ContactWidget::refreshList() {
    if (!m_listWidget) return;
    std::sort(m_visibleConversations.begin(), m_visibleConversations.end(),
              [](const Conversation& a, const Conversation& b) {
                  return a.updatedAt > b.updatedAt;
              });
    m_listWidget->clear();
    for (const auto& c : m_visibleConversations) {
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

    auto* root = new QHBoxLayout(container);
    root->setContentsMargins(10, 8, 10, 8);
    root->setSpacing(10);

    auto* avatar = new QLabel(container);
    avatar->setObjectName("conversationAvatar");
    const QString nameSeed = conversation.peerName.isEmpty() ? conversation.chatId : conversation.peerName;
    if (!conversation.peerAvatar.trimmed().isEmpty()) {
        QPixmap source(conversation.peerAvatar);
        if (!source.isNull()) {
            QPixmap circle = ImageUtils::makeCircular(source).scaled(
                38, 38, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            avatar->setPixmap(circle);
        } else {
            avatar->setText(nameSeed.left(1).toUpper());
        }
    } else {
        avatar->setText(nameSeed.left(1).toUpper());
    }
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setFixedSize(38, 38);
    root->addWidget(avatar);

    auto* contentWrap = new QVBoxLayout();
    contentWrap->setSpacing(4);
    contentWrap->setContentsMargins(0, 0, 0, 0);

    auto* top = new QHBoxLayout();
    top->setSpacing(6);
    auto* name = new QLabel(conversation.peerName.isEmpty() ? conversation.chatId : conversation.peerName, container);
    name->setObjectName("conversationName");
    name->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto* time = new QLabel(BuildTimeText(conversation.updatedAt), container);
    time->setObjectName("conversationTime");
    top->addWidget(name, 1);
    top->addWidget(time, 0, Qt::AlignRight);
    contentWrap->addLayout(top);

    auto* bottom = new QHBoxLayout();
    bottom->setSpacing(6);
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
    contentWrap->addLayout(bottom);
    root->addLayout(contentWrap, 1);

    container->setStyleSheet(
        "QFrame#conversationCard { background: transparent; border-radius: 8px; border: none; }"
        "QFrame#conversationCard[active='true'] { background: #dfdfdf; }"
        "QFrame#conversationCard:hover { background: #eaeaea; }"
        "QLabel#conversationAvatar { background: #7f9cbf; color: white; border-radius: 19px; font-size: 14px; font-weight: 700; }"
        "QLabel#conversationName { color: #1f1f1f; font-size: 14px; font-weight: 500; }"
        "QLabel#conversationTime { color: #9a9a9a; font-size: 11px; }"
        "QLabel#conversationPreview { color: #7c7c7c; font-size: 12px; }"
        "QLabel#unreadBadge { background: #ff5b5b; color: white; border-radius: 11px; font-size: 11px; font-weight: 700; padding: 0 6px; }"
    );
    return container;
}

void ContactWidget::onItemClicked(QListWidgetItem* item) {
    if (!item) return;
    const QString chatId = item->data(Qt::UserRole).toString();
    const int chatType = item->data(Qt::UserRole + 1).toInt();
    m_currentChatId = chatId;
    refreshList();
    emit conversationSelected(chatId, chatType);
}

void ContactWidget::applyFilter(const QString& keyword) {
    m_visibleConversations.clear();
    const QString key = keyword.trimmed();
    if (key.isEmpty()) {
        m_visibleConversations = m_conversations;
        return;
    }
    for (const auto& c : m_conversations) {
        const QString name = c.peerName.isEmpty() ? c.chatId : c.peerName;
        const QString preview = BuildPreview(c);
        if (name.contains(key, Qt::CaseInsensitive) ||
            c.chatId.contains(key, Qt::CaseInsensitive) ||
            preview.contains(key, Qt::CaseInsensitive)) {
            m_visibleConversations.append(c);
        }
    }
}

void ContactWidget::onSearchTextChanged(const QString& text) {
    applyFilter(text);
    refreshList();
}
