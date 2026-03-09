#include "contactwidget.h"

#include <QDate>
#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QComboBox>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <algorithm>

#include "utils/image_utils.h"

namespace {

QString BuildPreview(const Conversation& c) {
    if (c.lastMessage.status == 1) return QStringLiteral("[消息已撤回]");
    const QString text = c.lastMessage.content.trimmed();
    return text.isEmpty() ? QStringLiteral("暂无消息") : text;
}

QString BuildTimeText(qint64 timestamp) {
    if (timestamp <= 0) return QString();
    const QDateTime dt = QDateTime::fromMSecsSinceEpoch(timestamp);
    if (dt.date() == QDate::currentDate()) return dt.toString("HH:mm");
    return dt.toString("MM-dd");
}

QString DisplayName(const QString& nickname, const QString& fallback) {
    return nickname.trimmed().isEmpty() ? fallback : nickname;
}

void ApplyAvatarToLabel(QLabel* avatar, const QString& avatarUrl, const QString& fallbackName) {
    if (!avatar) return;
    if (!avatarUrl.trimmed().isEmpty()) {
        QPixmap source(avatarUrl);
        if (!source.isNull()) {
            QPixmap circle = ImageUtils::makeCircular(source).scaled(
                38, 38, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            avatar->setPixmap(circle);
            avatar->setText("");
            return;
        }
    }
    avatar->setPixmap(QPixmap());
    avatar->setText(fallbackName.left(1).toUpper());
}

}  // namespace

ContactWidget::ContactWidget(QWidget *parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 8);
    root->setSpacing(8);

    auto* topTabs = new QFrame(this);
    topTabs->setObjectName("topTabs");
    auto* tabLayout = new QHBoxLayout(topTabs);
    tabLayout->setContentsMargins(0, 0, 0, 0);
    tabLayout->setSpacing(6);
    m_chatTabBtn = new QPushButton("聊天", topTabs);
    m_friendTabBtn = new QPushButton("好友", topTabs);
    m_groupTabBtn = new QPushButton("群聊", topTabs);
    for (auto* btn : {m_chatTabBtn, m_friendTabBtn, m_groupTabBtn}) {
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        tabLayout->addWidget(btn);
    }
    root->addWidget(topTabs);

    auto* searchWrap = new QFrame(this);
    searchWrap->setObjectName("searchWrap");
    auto* searchLayout = new QHBoxLayout(searchWrap);
    searchLayout->setContentsMargins(8, 4, 8, 4);
    m_searchEdit = new QLineEdit(searchWrap);
    m_searchEdit->setPlaceholderText("搜索");
    m_searchEdit->setClearButtonEnabled(true);
    searchLayout->addWidget(m_searchEdit);
    root->addWidget(searchWrap);

    m_stack = new QStackedWidget(this);
    root->addWidget(m_stack, 1);

    auto* convPage = new QWidget(m_stack);
    auto* convLayout = new QVBoxLayout(convPage);
    convLayout->setContentsMargins(0, 0, 0, 0);
    m_convListWidget = new QListWidget(convPage);
    m_convListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_convListWidget->setFrameShape(QFrame::NoFrame);
    m_convListWidget->setSpacing(2);
    m_convListWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_convListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    convLayout->addWidget(m_convListWidget);
    m_stack->addWidget(convPage);

    auto* friendPage = new QWidget(m_stack);
    auto* friendLayout = new QVBoxLayout(friendPage);
    friendLayout->setContentsMargins(0, 0, 0, 0);
    friendLayout->setSpacing(8);
    auto* requestTitle = new QLabel("新的朋友", friendPage);
    requestTitle->setObjectName("sectionTitle");
    m_friendRequestListWidget = new QListWidget(friendPage);
    m_friendRequestListWidget->setFrameShape(QFrame::NoFrame);
    m_friendRequestListWidget->setSpacing(4);
    m_friendRequestListWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_friendRequestListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_friendRequestListWidget->setMaximumHeight(180);
    auto* friendTitle = new QLabel("我的好友", friendPage);
    friendTitle->setObjectName("sectionTitle");
    m_friendListWidget = new QListWidget(friendPage);
    m_friendListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_friendListWidget->setFrameShape(QFrame::NoFrame);
    m_friendListWidget->setSpacing(2);
    m_friendListWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_friendListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    friendLayout->addWidget(requestTitle);
    friendLayout->addWidget(m_friendRequestListWidget);
    friendLayout->addWidget(friendTitle);
    friendLayout->addWidget(m_friendListWidget, 1);
    m_stack->addWidget(friendPage);

    auto* groupPage = new QWidget(m_stack);
    auto* groupLayout = new QVBoxLayout(groupPage);
    groupLayout->setContentsMargins(0, 0, 0, 0);
    groupLayout->setSpacing(8);
    auto* groupTitle = new QLabel("我的群聊", groupPage);
    groupTitle->setObjectName("sectionTitle");
    m_groupListWidget = new QListWidget(groupPage);
    m_groupListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_groupListWidget->setFrameShape(QFrame::NoFrame);
    m_groupListWidget->setSpacing(2);
    m_groupListWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_groupListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_groupListWidget->setMaximumHeight(220);
    auto* infoCard = new QFrame(groupPage);
    infoCard->setObjectName("groupInfoCard");
    auto* infoLayout = new QVBoxLayout(infoCard);
    infoLayout->setContentsMargins(10, 8, 10, 8);
    infoLayout->setSpacing(4);
    m_groupNameLabel = new QLabel("未选择群聊", infoCard);
    m_groupNameLabel->setObjectName("groupName");
    m_groupMetaLabel = new QLabel("", infoCard);
    m_groupMetaLabel->setObjectName("groupMeta");
    m_groupAnnouncementLabel = new QLabel("", infoCard);
    m_groupAnnouncementLabel->setWordWrap(true);
    m_groupAnnouncementLabel->setObjectName("groupMeta");
    infoLayout->addWidget(m_groupNameLabel);
    infoLayout->addWidget(m_groupMetaLabel);
    infoLayout->addWidget(m_groupAnnouncementLabel);
    auto* memberTitle = new QLabel("群成员", groupPage);
    memberTitle->setObjectName("sectionTitle");
    auto* memberFilterRow = new QHBoxLayout();
    memberFilterRow->setSpacing(6);
    m_groupMemberSearchEdit = new QLineEdit(groupPage);
    m_groupMemberSearchEdit->setPlaceholderText("搜索成员");
    m_groupMemberSearchEdit->setClearButtonEnabled(true);
    m_groupRoleFilter = new QComboBox(groupPage);
    m_groupRoleFilter->addItem("全部角色", -1);
    m_groupRoleFilter->addItem("群主", 2);
    m_groupRoleFilter->addItem("管理员", 1);
    m_groupRoleFilter->addItem("成员", 0);
    memberFilterRow->addWidget(m_groupMemberSearchEdit, 1);
    memberFilterRow->addWidget(m_groupRoleFilter);
    m_groupMemberListWidget = new QListWidget(groupPage);
    m_groupMemberListWidget->setFrameShape(QFrame::NoFrame);
    m_groupMemberListWidget->setSpacing(2);
    m_groupMemberListWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_groupMemberListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    groupLayout->addWidget(groupTitle);
    groupLayout->addWidget(m_groupListWidget);
    groupLayout->addWidget(infoCard);
    groupLayout->addWidget(memberTitle);
    groupLayout->addLayout(memberFilterRow);
    groupLayout->addWidget(m_groupMemberListWidget, 1);
    m_stack->addWidget(groupPage);

    setStyleSheet(
        "ContactWidget { background: #f5f5f5; border-right: 1px solid #dfdfdf; }"
        "QFrame#topTabs { background: transparent; }"
        "QPushButton { background: #ebedf0; border: none; border-radius: 8px; color: #444; font-size: 12px; padding: 6px 0; }"
        "QPushButton:checked { background: #d9e8ff; color: #2468f2; font-weight: 600; }"
        "QFrame#searchWrap { background: #ffffff; border: 1px solid #dedede; border-radius: 8px; }"
        "QLineEdit { border: none; background: transparent; color: #303133; font-size: 13px; padding: 4px 2px; }"
        "QListWidget { background: transparent; border: none; outline: none; }"
        "QListWidget::item { border: none; padding: 0px; margin: 0px; }"
        "QListWidget::item:selected { background: transparent; }"
        "QLabel#sectionTitle { color: #666; font-size: 12px; padding-left: 2px; }"
        "QFrame#groupInfoCard { background: #ffffff; border: 1px solid #e4e8ef; border-radius: 8px; }"
        "QLabel#groupName { color: #222; font-size: 14px; font-weight: 600; }"
        "QLabel#groupMeta { color: #7e8694; font-size: 12px; }"
        "QComboBox { background:#fff; border:1px solid #d8dde8; border-radius:6px; padding:4px 8px; font-size:12px; }"
    );

    connect(m_chatTabBtn, &QPushButton::clicked, this, [this]() { setViewMode(ViewMode::Conversations); });
    connect(m_friendTabBtn, &QPushButton::clicked, this, [this]() { setViewMode(ViewMode::Friends); });
    connect(m_groupTabBtn, &QPushButton::clicked, this, [this]() { setViewMode(ViewMode::Groups); });
    connect(m_searchEdit, &QLineEdit::textChanged, this, &ContactWidget::onSearchTextChanged);
    connect(m_convListWidget, &QListWidget::itemClicked, this, &ContactWidget::onItemClicked);
    connect(m_friendListWidget, &QListWidget::itemClicked, this, &ContactWidget::onFriendItemClicked);
    connect(m_groupListWidget, &QListWidget::itemClicked, this, &ContactWidget::onGroupItemClicked);
    connect(m_groupMemberSearchEdit, &QLineEdit::textChanged, this, &ContactWidget::onGroupMemberSearchChanged);
    connect(m_groupRoleFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ContactWidget::onGroupRoleFilterChanged);
    setViewMode(ViewMode::Conversations);
}

ContactWidget::~ContactWidget() = default;

void ContactWidget::setViewMode(ViewMode mode) {
    m_viewMode = mode;
    if (m_stack) {
        m_stack->setCurrentIndex(static_cast<int>(mode));
    }
    updateTabStyles();
    applyFilter(m_searchEdit ? m_searchEdit->text() : QString(), mode);
    refreshView();
}

void ContactWidget::setConversations(const QList<Conversation>& conversations) {
    m_conversations = conversations;
    applyFilter(m_searchEdit ? m_searchEdit->text() : QString(), ViewMode::Conversations);
    if (m_viewMode == ViewMode::Conversations) refreshConversationList();
}

void ContactWidget::upsertConversation(const Conversation& conversation) {
    for (int i = 0; i < m_conversations.size(); ++i) {
        if (m_conversations[i].chatId == conversation.chatId &&
            m_conversations[i].chatType == conversation.chatType) {
            m_conversations[i] = conversation;
            applyFilter(m_searchEdit ? m_searchEdit->text() : QString(), ViewMode::Conversations);
            if (m_viewMode == ViewMode::Conversations) refreshConversationList();
            return;
        }
    }
    m_conversations.prepend(conversation);
    applyFilter(m_searchEdit ? m_searchEdit->text() : QString(), ViewMode::Conversations);
    if (m_viewMode == ViewMode::Conversations) refreshConversationList();
}

void ContactWidget::increaseUnreadForChat(const QString& chatId) {
    for (auto& c : m_conversations) {
        if (c.chatId == chatId) {
            c.unreadCount += 1;
            applyFilter(m_searchEdit ? m_searchEdit->text() : QString(), ViewMode::Conversations);
            if (m_viewMode == ViewMode::Conversations) refreshConversationList();
            return;
        }
    }
}

void ContactWidget::setCurrentChat(const QString& chatId) {
    m_currentChatId = chatId;
    for (auto& c : m_conversations) {
        if (c.chatId == chatId) c.unreadCount = 0;
    }
    applyFilter(m_searchEdit ? m_searchEdit->text() : QString(), ViewMode::Conversations);
    if (m_viewMode == ViewMode::Conversations) refreshConversationList();
}

void ContactWidget::setFriends(const QList<FriendItem>& friends) {
    m_friends = friends;
    applyFilter(m_searchEdit ? m_searchEdit->text() : QString(), ViewMode::Friends);
    if (m_viewMode == ViewMode::Friends) refreshFriendList();
}

void ContactWidget::setFriendRequests(const QList<FriendRequestItem>& requests) {
    m_friendRequests = requests;
    std::sort(m_friendRequests.begin(), m_friendRequests.end(),
              [](const FriendRequestItem& a, const FriendRequestItem& b) {
                  return a.createdAt > b.createdAt;
              });
    if (m_viewMode == ViewMode::Friends) refreshFriendList();
}

void ContactWidget::setGroups(const QList<GroupItem>& groups) {
    m_groups = groups;
    applyFilter(m_searchEdit ? m_searchEdit->text() : QString(), ViewMode::Groups);
    if (m_viewMode == ViewMode::Groups) refreshGroupList();
}

void ContactWidget::setCurrentGroupInfo(const GroupItem& groupInfo) {
    m_currentGroupInfo = groupInfo;
    if (m_groupNameLabel) {
        m_groupNameLabel->setText(groupInfo.groupName.isEmpty() ? groupInfo.groupId : groupInfo.groupName);
    }
    if (m_groupMetaLabel) {
        m_groupMetaLabel->setText(QString("群ID: %1  ·  成员: %2").arg(groupInfo.groupId).arg(groupInfo.memberCount));
    }
    if (m_groupAnnouncementLabel) {
        m_groupAnnouncementLabel->setText(groupInfo.announcement.isEmpty()
                                          ? QStringLiteral("暂无群公告")
                                          : QString("公告：%1").arg(groupInfo.announcement));
    }
}

void ContactWidget::setGroupMembers(const QList<GroupMemberItem>& members) {
    m_groupMembers = members;
    applyGroupMemberFilter();
    if (m_viewMode == ViewMode::Groups) refreshGroupMemberList();
}

ContactWidget::FriendItem ContactWidget::friendById(const QString& userId) const {
    for (const auto& f : m_friends) {
        if (f.friendId == userId) return f;
    }
    return FriendItem{};
}

void ContactWidget::updateFriendRemark(const QString& userId, const QString& remark) {
    for (auto& f : m_friends) {
        if (f.friendId == userId) {
            f.remark = remark;
            break;
        }
    }
    applyFilter(m_searchEdit ? m_searchEdit->text() : QString(), ViewMode::Friends);
    if (m_viewMode == ViewMode::Friends) refreshFriendList();
}

void ContactWidget::removeFriendById(const QString& userId) {
    for (int i = 0; i < m_friends.size(); ++i) {
        if (m_friends[i].friendId == userId) {
            m_friends.removeAt(i);
            break;
        }
    }
    applyFilter(m_searchEdit ? m_searchEdit->text() : QString(), ViewMode::Friends);
    if (m_viewMode == ViewMode::Friends) refreshFriendList();
}

void ContactWidget::refreshView() {
    if (m_viewMode == ViewMode::Conversations) refreshConversationList();
    else if (m_viewMode == ViewMode::Friends) refreshFriendList();
    else refreshGroupList();
}

void ContactWidget::refreshConversationList() {
    if (!m_convListWidget) return;
    std::sort(m_visibleConversations.begin(), m_visibleConversations.end(),
              [](const Conversation& a, const Conversation& b) { return a.updatedAt > b.updatedAt; });
    m_convListWidget->clear();
    for (const auto& c : m_visibleConversations) {
        auto* item = new QListWidgetItem(m_convListWidget);
        item->setData(Qt::UserRole, c.chatId);
        item->setData(Qt::UserRole + 1, c.chatType);
        auto* rowWidget = buildConversationItemWidget(c);
        item->setSizeHint(rowWidget->sizeHint());
        m_convListWidget->setItemWidget(item, rowWidget);
    }
}

void ContactWidget::refreshFriendList() {
    if (!m_friendListWidget || !m_friendRequestListWidget) return;
    m_friendRequestListWidget->clear();
    for (const auto& req : m_friendRequests) {
        auto* item = new QListWidgetItem(m_friendRequestListWidget);
        auto* rowWidget = buildFriendRequestItemWidget(req);
        item->setSizeHint(rowWidget->sizeHint());
        m_friendRequestListWidget->setItemWidget(item, rowWidget);
    }
    m_friendListWidget->clear();
    for (const auto& f : m_visibleFriends) {
        auto* item = new QListWidgetItem(m_friendListWidget);
        item->setData(Qt::UserRole, f.friendId);
        auto* rowWidget = buildFriendItemWidget(f);
        item->setSizeHint(rowWidget->sizeHint());
        m_friendListWidget->setItemWidget(item, rowWidget);
    }
}

void ContactWidget::refreshGroupList() {
    if (!m_groupListWidget) return;
    m_groupListWidget->clear();
    for (const auto& g : m_visibleGroups) {
        auto* item = new QListWidgetItem(m_groupListWidget);
        item->setData(Qt::UserRole, g.groupId);
        auto* rowWidget = buildGroupItemWidget(g);
        item->setSizeHint(rowWidget->sizeHint());
        m_groupListWidget->setItemWidget(item, rowWidget);
        if (!m_currentGroupId.isEmpty() && g.groupId == m_currentGroupId) {
            m_groupListWidget->setCurrentItem(item);
        }
    }
    refreshGroupMemberList();
}

void ContactWidget::refreshGroupMemberList() {
    if (!m_groupMemberListWidget) return;
    m_groupMemberListWidget->clear();
    for (const auto& m : m_visibleGroupMembers) {
        auto* item = new QListWidgetItem(m_groupMemberListWidget);
        auto* rowWidget = buildGroupMemberItemWidget(m);
        item->setSizeHint(rowWidget->sizeHint());
        m_groupMemberListWidget->setItemWidget(item, rowWidget);
    }
}

void ContactWidget::applyGroupMemberFilter() {
    m_visibleGroupMembers.clear();
    const QString key = m_groupMemberSearchEdit ? m_groupMemberSearchEdit->text().trimmed() : QString();
    const int roleFilter = m_groupRoleFilter ? m_groupRoleFilter->currentData().toInt() : -1;
    for (const auto& m : m_groupMembers) {
        if (roleFilter >= 0 && m.role != roleFilter) continue;
        const QString name = DisplayName(m.nickname, m.userId);
        if (!key.isEmpty() &&
            !name.contains(key, Qt::CaseInsensitive) &&
            !m.userId.contains(key, Qt::CaseInsensitive)) {
            continue;
        }
        m_visibleGroupMembers.append(m);
    }
}

void ContactWidget::applyFilter(const QString& keyword, ViewMode mode) {
    const QString key = keyword.trimmed();
    if (mode == ViewMode::Conversations) {
        m_visibleConversations.clear();
        if (key.isEmpty()) {
            m_visibleConversations = m_conversations;
            return;
        }
        for (const auto& c : m_conversations) {
            const QString name = c.peerName.isEmpty() ? c.chatId : c.peerName;
            if (name.contains(key, Qt::CaseInsensitive) ||
                c.chatId.contains(key, Qt::CaseInsensitive) ||
                BuildPreview(c).contains(key, Qt::CaseInsensitive)) {
                m_visibleConversations.append(c);
            }
        }
        return;
    }
    if (mode == ViewMode::Friends) {
        m_visibleFriends.clear();
        if (key.isEmpty()) {
            m_visibleFriends = m_friends;
            return;
        }
        for (const auto& f : m_friends) {
            const QString showName = DisplayName(f.remark, DisplayName(f.nickname, f.friendId));
            if (showName.contains(key, Qt::CaseInsensitive) ||
                f.friendId.contains(key, Qt::CaseInsensitive) ||
                f.nickname.contains(key, Qt::CaseInsensitive)) {
                m_visibleFriends.append(f);
            }
        }
        return;
    }
    m_visibleGroups.clear();
    if (key.isEmpty()) {
        m_visibleGroups = m_groups;
        return;
    }
    for (const auto& g : m_groups) {
        if (g.groupName.contains(key, Qt::CaseInsensitive) ||
            g.groupId.contains(key, Qt::CaseInsensitive)) {
            m_visibleGroups.append(g);
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
    const QString nameSeed = DisplayName(conversation.peerName, conversation.chatId);
    ApplyAvatarToLabel(avatar, conversation.peerAvatar, nameSeed);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setFixedSize(38, 38);
    root->addWidget(avatar);

    auto* contentWrap = new QVBoxLayout();
    contentWrap->setSpacing(4);
    contentWrap->setContentsMargins(0, 0, 0, 0);
    auto* top = new QHBoxLayout();
    auto* name = new QLabel(nameSeed, container);
    name->setObjectName("conversationName");
    auto* time = new QLabel(BuildTimeText(conversation.updatedAt), container);
    time->setObjectName("conversationTime");
    top->addWidget(name, 1);
    top->addWidget(time);
    contentWrap->addLayout(top);

    auto* bottom = new QHBoxLayout();
    auto* preview = new QLabel(BuildPreview(conversation), container);
    preview->setObjectName("conversationPreview");
    bottom->addWidget(preview, 1);
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

QWidget* ContactWidget::buildFriendItemWidget(const FriendItem& item) const {
    auto* container = new QFrame();
    container->setObjectName("friendCard");
    auto* root = new QHBoxLayout(container);
    root->setContentsMargins(10, 8, 10, 8);
    root->setSpacing(10);
    auto* avatar = new QLabel(container);
    avatar->setObjectName("friendAvatar");
    const QString display = DisplayName(item.remark, DisplayName(item.nickname, item.friendId));
    ApplyAvatarToLabel(avatar, item.avatarUrl, display);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setFixedSize(36, 36);
    auto* textWrap = new QVBoxLayout();
    textWrap->setSpacing(2);
    textWrap->setContentsMargins(0, 0, 0, 0);
    auto* name = new QLabel(display, container);
    name->setObjectName("friendName");
    auto* id = new QLabel(QString("ID: %1").arg(item.friendId), container);
    id->setObjectName("friendMeta");
    textWrap->addWidget(name);
    textWrap->addWidget(id);
    root->addWidget(avatar);
    root->addLayout(textWrap, 1);
    container->setStyleSheet(
        "QFrame#friendCard { background: transparent; border-radius: 8px; }"
        "QFrame#friendCard:hover { background: #ebebeb; }"
        "QLabel#friendAvatar { background: #6ca7e8; color: white; border-radius: 18px; font-size: 13px; font-weight: 700; }"
        "QLabel#friendName { color: #252525; font-size: 14px; font-weight: 500; }"
        "QLabel#friendMeta { color: #7f8694; font-size: 11px; }"
    );
    return container;
}

QWidget* ContactWidget::buildFriendRequestItemWidget(const FriendRequestItem& item) {
    auto* container = new QFrame();
    container->setObjectName("requestCard");
    auto* root = new QHBoxLayout(container);
    root->setContentsMargins(10, 8, 10, 8);
    root->setSpacing(8);

    auto* avatar = new QLabel(container);
    avatar->setObjectName("requestAvatar");
    const QString name = DisplayName(item.fromNickname, item.fromUserId);
    ApplyAvatarToLabel(avatar, item.fromAvatarUrl, name);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setFixedSize(34, 34);

    auto* textWrap = new QVBoxLayout();
    textWrap->setSpacing(2);
    auto* nameLabel = new QLabel(name, container);
    nameLabel->setObjectName("requestName");
    auto* remark = new QLabel(item.remark.isEmpty() ? QStringLiteral("请求添加你为好友") : item.remark, container);
    remark->setObjectName("requestMeta");
    remark->setWordWrap(true);
    textWrap->addWidget(nameLabel);
    textWrap->addWidget(remark);

    root->addWidget(avatar);
    root->addLayout(textWrap, 1);

    const bool isOutgoing = !m_currentUserId.isEmpty() && item.fromUserId == m_currentUserId;
    const bool isIncoming = !isOutgoing;
    if (item.status == 0 && isIncoming) {
        auto* agreeBtn = new QPushButton("同意", container);
        agreeBtn->setCursor(Qt::PointingHandCursor);
        auto* rejectBtn = new QPushButton("拒绝", container);
        rejectBtn->setCursor(Qt::PointingHandCursor);
        connect(agreeBtn, &QPushButton::clicked, this, [this, reqId = item.requestId]() {
            emit friendRequestHandled(reqId, true);
        });
        connect(rejectBtn, &QPushButton::clicked, this, [this, reqId = item.requestId]() {
            emit friendRequestHandled(reqId, false);
        });
        root->addWidget(agreeBtn);
        root->addWidget(rejectBtn);
    } else {
        QString stateText;
        if (item.status == 0) {
            stateText = QStringLiteral("等待同意");
        } else if (item.status == 1) {
            stateText = isIncoming ? QStringLiteral("已同意") : QStringLiteral("已添加");
        } else {
            stateText = QStringLiteral("已拒绝");
        }
        auto* done = new QLabel(stateText, container);
        done->setObjectName("requestMeta");
        root->addWidget(done);
    }

    container->setStyleSheet(
        "QFrame#requestCard { background: #ffffff; border: 1px solid #e4e7ee; border-radius: 8px; }"
        "QLabel#requestAvatar { background: #7ca7d9; color: white; border-radius: 17px; font-size: 12px; font-weight: 700; }"
        "QLabel#requestName { color: #252525; font-size: 13px; font-weight: 600; }"
        "QLabel#requestMeta { color: #7f8694; font-size: 11px; }"
        "QPushButton { background: #eef4ff; color: #2b62d9; border: none; border-radius: 6px; padding: 4px 10px; font-size: 12px; }"
        "QPushButton:hover { background: #dce9ff; }"
    );
    return container;
}

QWidget* ContactWidget::buildGroupItemWidget(const GroupItem& item) const {
    auto* container = new QFrame();
    container->setObjectName("groupCard");
    container->setProperty("active", item.groupId == m_currentGroupId);
    auto* root = new QHBoxLayout(container);
    root->setContentsMargins(10, 8, 10, 8);
    root->setSpacing(10);
    auto* avatar = new QLabel(container);
    avatar->setObjectName("groupAvatar");
    const QString name = DisplayName(item.groupName, item.groupId);
    ApplyAvatarToLabel(avatar, item.avatarUrl, name);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setFixedSize(36, 36);
    auto* textWrap = new QVBoxLayout();
    textWrap->setSpacing(2);
    auto* nameLabel = new QLabel(name, container);
    nameLabel->setObjectName("groupItemName");
    auto* meta = new QLabel(QString("成员 %1").arg(item.memberCount), container);
    meta->setObjectName("groupItemMeta");
    textWrap->addWidget(nameLabel);
    textWrap->addWidget(meta);
    root->addWidget(avatar);
    root->addLayout(textWrap, 1);
    container->setStyleSheet(
        "QFrame#groupCard { background: transparent; border-radius: 8px; }"
        "QFrame#groupCard[active='true'] { background: #dfe8ff; }"
        "QFrame#groupCard:hover { background: #eceff5; }"
        "QLabel#groupAvatar { background: #8a99bb; color: white; border-radius: 18px; font-size: 13px; font-weight: 700; }"
        "QLabel#groupItemName { color: #252525; font-size: 13px; font-weight: 600; }"
        "QLabel#groupItemMeta { color: #8a909c; font-size: 11px; }"
    );
    return container;
}

QWidget* ContactWidget::buildGroupMemberItemWidget(const GroupMemberItem& item) const {
    auto* container = new QFrame();
    auto* root = new QHBoxLayout(container);
    root->setContentsMargins(8, 6, 8, 6);
    auto* name = new QLabel(DisplayName(item.nickname, item.userId), container);
    name->setStyleSheet("color:#2a2a2a;font-size:12px;");
    auto* role = new QLabel(item.role == 2 ? "群主" : (item.role == 1 ? "管理员" : "成员"), container);
    role->setStyleSheet("color:#8b92a0;font-size:11px;");
    root->addWidget(name, 1);
    root->addWidget(role);
    return container;
}

void ContactWidget::onItemClicked(QListWidgetItem* item) {
    if (!item) return;
    const QString chatId = item->data(Qt::UserRole).toString();
    const int chatType = item->data(Qt::UserRole + 1).toInt();
    m_currentChatId = chatId;
    refreshConversationList();
    emit conversationSelected(chatId, chatType);
}

void ContactWidget::onFriendItemClicked(QListWidgetItem* item) {
    if (!item) return;
    const QString userId = item->data(Qt::UserRole).toString();
    emit friendSelected(userId);
}

void ContactWidget::onGroupItemClicked(QListWidgetItem* item) {
    if (!item) return;
    m_currentGroupId = item->data(Qt::UserRole).toString();
    refreshGroupList();
    emit groupSelected(m_currentGroupId);
    emit conversationSelected(m_currentGroupId, 2);
}

void ContactWidget::onSearchTextChanged(const QString& text) {
    applyFilter(text, m_viewMode);
    refreshView();
}

void ContactWidget::onGroupMemberSearchChanged(const QString& text) {
    Q_UNUSED(text);
    applyGroupMemberFilter();
    refreshGroupMemberList();
}

void ContactWidget::onGroupRoleFilterChanged(int index) {
    Q_UNUSED(index);
    applyGroupMemberFilter();
    refreshGroupMemberList();
}

void ContactWidget::updateTabStyles() {
    if (!m_chatTabBtn || !m_friendTabBtn || !m_groupTabBtn) return;
    m_chatTabBtn->setChecked(m_viewMode == ViewMode::Conversations);
    m_friendTabBtn->setChecked(m_viewMode == ViewMode::Friends);
    m_groupTabBtn->setChecked(m_viewMode == ViewMode::Groups);
}
