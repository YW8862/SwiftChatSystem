#include "mainwindow.h"

#include "contactwidget.h"
#include "chatwidget.h"
#include "network/app_service.h"
#include "network/protocol_handler.h"
#include "zone.pb.h"
#include "gate.pb.h"

#include <QDateTime>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QSaveFile>
#include <QStackedWidget>
#include <QSplitter>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QEventLoop>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QFile>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include "utils/image_utils.h"

MainWindow::MainWindow(ProtocolHandler* protocol,
                       const QString& currentUserId,
                       QWidget *parent)
    : QMainWindow(parent)
    , m_protocol(protocol)
    , m_currentUserId(currentUserId) {
    setWindowTitle("SwiftChat");
    resize(1024, 768);

    setupUi();
    m_networkManager = new QNetworkAccessManager(this);
    m_appService = std::make_unique<client::AppService>(m_protocol);
    wireSignals();
    syncConversations();
    loadFriends();
    loadFriendRequests();
    loadUserGroups();
    pullOfflineMessages();
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
    m_rightStack = new QStackedWidget(central);
    m_chatWidget = new ChatWidget(m_rightStack);
    m_chatWidget->setCurrentUserId(m_currentUserId);
    m_friendProfilePage = new QWidget(m_rightStack);
    auto* profileLayout = new QVBoxLayout(m_friendProfilePage);
    profileLayout->setContentsMargins(32, 28, 32, 28);
    profileLayout->setSpacing(12);
    auto* topRow = new QHBoxLayout();
    auto* title = new QLabel("好友资料", m_friendProfilePage);
    title->setStyleSheet("color:#1f1f1f;font-size:20px;font-weight:700;");
    m_profileMoreBtn = new QToolButton(m_friendProfilePage);
    m_profileMoreBtn->setText("⋯");
    m_profileMoreBtn->setCursor(Qt::PointingHandCursor);
    m_profileMoreBtn->setStyleSheet("QToolButton{border:none;background:transparent;font-size:20px;color:#6d7481;}QToolButton:hover{background:#eceff5;border-radius:8px;}");
    topRow->addWidget(title);
    topRow->addStretch();
    topRow->addWidget(m_profileMoreBtn);

    auto* profileHead = new QHBoxLayout();
    m_profileAvatarLabel = new QLabel("-", m_friendProfilePage);
    m_profileAvatarLabel->setFixedSize(68, 68);
    m_profileAvatarLabel->setAlignment(Qt::AlignCenter);
    m_profileAvatarLabel->setStyleSheet("background:#6fa2ec;color:white;border-radius:34px;font-size:26px;font-weight:700;");
    auto* headTextWrap = new QVBoxLayout();
    m_profileNameLabel = new QLabel("-", m_friendProfilePage);
    m_profileNameLabel->setStyleSheet("color:#2c2c2c;font-size:18px;font-weight:600;");
    auto* tagRow = new QHBoxLayout();
    tagRow->setSpacing(6);
    m_profileTagStarLabel = new QLabel("普通好友", m_friendProfilePage);
    m_profileTagGroupLabel = new QLabel("默认分组", m_friendProfilePage);
    m_profileTagStarLabel->setStyleSheet("background:#fff4d9;color:#9d6700;border-radius:10px;padding:2px 8px;font-size:11px;");
    m_profileTagGroupLabel->setStyleSheet("background:#e8efff;color:#2752b4;border-radius:10px;padding:2px 8px;font-size:11px;");
    tagRow->addWidget(m_profileTagStarLabel);
    tagRow->addWidget(m_profileTagGroupLabel);
    tagRow->addStretch();
    headTextWrap->addWidget(m_profileNameLabel);
    headTextWrap->addLayout(tagRow);
    profileHead->addWidget(m_profileAvatarLabel);
    profileHead->addLayout(headTextWrap, 1);

    m_profileIdLabel = new QLabel("ID: -", m_friendProfilePage);
    m_profileRemarkLabel = new QLabel("备注: -", m_friendProfilePage);
    m_profileSourceLabel = new QLabel("来源: 好友列表同步", m_friendProfilePage);
    m_profileAddedAtLabel = new QLabel("加好友时间: -", m_friendProfilePage);
    for (auto* label : {m_profileIdLabel, m_profileRemarkLabel, m_profileSourceLabel, m_profileAddedAtLabel}) {
        label->setStyleSheet("color:#5f6776;font-size:13px;");
    }
    auto* card = new QFrame(m_friendProfilePage);
    card->setObjectName("profileCard");
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(18, 16, 18, 16);
    cardLayout->setSpacing(8);
    cardLayout->addLayout(profileHead);
    cardLayout->addWidget(m_profileIdLabel);
    cardLayout->addWidget(m_profileRemarkLabel);
    cardLayout->addWidget(m_profileSourceLabel);
    cardLayout->addWidget(m_profileAddedAtLabel);
    m_profileSendMsgBtn = new QPushButton("发消息", m_friendProfilePage);
    m_profileSendMsgBtn->setCursor(Qt::PointingHandCursor);
    m_profileSendMsgBtn->setMinimumHeight(38);
    m_profileSendMsgBtn->setStyleSheet(
        "QPushButton{background:#07c160;color:white;border:none;border-radius:8px;font-size:14px;font-weight:600;padding:0 18px;}"
        "QPushButton:hover{background:#06ad56;}"
    );
    profileLayout->addLayout(topRow);
    profileLayout->addWidget(card);
    profileLayout->addWidget(m_profileSendMsgBtn, 0, Qt::AlignLeft);
    profileLayout->addStretch();
    m_friendProfilePage->setStyleSheet(
        "QWidget{background:#f5f5f5;}"
        "QFrame#profileCard{background:#ffffff;border:1px solid #e1e6ef;border-radius:10px;}"
    );

    m_rightStack->addWidget(m_chatWidget);
    m_rightStack->addWidget(m_friendProfilePage);
    m_rightStack->setCurrentWidget(m_chatWidget);

    splitter->addWidget(navPanel);
    splitter->addWidget(m_contactWidget);
    splitter->addWidget(m_rightStack);
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
    connect(chatBtn, &QPushButton::clicked, this, [this, chatBtn, contactBtn]() {
        m_contactWidget->setViewMode(ContactWidget::ViewMode::Conversations);
        chatBtn->setObjectName("navBtnActive");
        contactBtn->setObjectName("navBtn");
        style()->unpolish(chatBtn);
        style()->polish(chatBtn);
        style()->unpolish(contactBtn);
        style()->polish(contactBtn);
    });
    connect(contactBtn, &QPushButton::clicked, this, [this, chatBtn, contactBtn]() {
        m_contactWidget->setViewMode(ContactWidget::ViewMode::Friends);
        contactBtn->setObjectName("navBtnActive");
        chatBtn->setObjectName("navBtn");
        style()->unpolish(chatBtn);
        style()->polish(chatBtn);
        style()->unpolish(contactBtn);
        style()->polish(contactBtn);
    });
    connect(m_profileSendMsgBtn, &QPushButton::clicked, this, [this]() {
        if (m_profileUserId.isEmpty()) return;
        Conversation conv;
        const QString key = convKey(m_profileUserId, 1);
        if (m_conversationMap.contains(key)) {
            conv = m_conversationMap.value(key);
        } else {
            conv.chatId = m_profileUserId;
            conv.chatType = 1;
            conv.peerId = m_profileUserId;
            conv.peerName = m_profileUserId;
            conv.updatedAt = QDateTime::currentMSecsSinceEpoch();
            m_conversationMap[key] = conv;
            m_contactWidget->upsertConversation(conv);
        }
        m_contactWidget->setViewMode(ContactWidget::ViewMode::Conversations);
        m_rightStack->setCurrentWidget(m_chatWidget);
        onConversationSelected(m_profileUserId, 1);
    });
    connect(m_profileMoreBtn, &QToolButton::clicked, this, [this]() {
        if (m_profileUserId.isEmpty()) return;
        QMenu menu(this);
        QAction* starAction = menu.addAction(m_starFriendIds.contains(m_profileUserId) ? "取消星标好友" : "设为星标好友");
        QAction* editRemarkAction = menu.addAction("编辑备注");
        QAction* deleteAction = menu.addAction("删除好友");
        QAction* picked = menu.exec(m_profileMoreBtn->mapToGlobal(QPoint(0, m_profileMoreBtn->height())));
        if (!picked) return;
        if (picked == starAction) {
            if (m_starFriendIds.contains(m_profileUserId)) m_starFriendIds.remove(m_profileUserId);
            else m_starFriendIds.insert(m_profileUserId);
            refreshFriendProfileCard();
            return;
        }
        if (picked == editRemarkAction) {
            bool ok = false;
            const QString initial = m_profileRemark.isEmpty() ? m_profileNickname : m_profileRemark;
            const QString remark = QInputDialog::getText(this, "编辑备注", "备注名：", QLineEdit::Normal, initial, &ok).trimmed();
            if (!ok) return;
            m_profileRemark = remark;
            if (m_contactWidget) m_contactWidget->updateFriendRemark(m_profileUserId, remark);
            refreshFriendProfileCard();
            return;
        }
        if (picked == deleteAction) {
            removeCurrentFriend();
        }
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
    connect(m_contactWidget, &ContactWidget::friendSelected,
            this, &MainWindow::onFriendSelected);
    connect(m_contactWidget, &ContactWidget::groupSelected,
            this, &MainWindow::onGroupSelected);
    connect(m_contactWidget, &ContactWidget::createGroupRequested,
            this, &MainWindow::createGroup);
    connect(m_contactWidget, &ContactWidget::inviteGroupMembersRequested,
            this, &MainWindow::inviteGroupMembers);
    connect(m_contactWidget, &ContactWidget::removeGroupMemberRequested,
            this, &MainWindow::removeGroupMember);
    connect(m_contactWidget, &ContactWidget::leaveGroupRequested,
            this, &MainWindow::leaveGroup);
    connect(m_contactWidget, &ContactWidget::friendRequestHandled,
            this, &MainWindow::handleFriendRequest);
    connect(m_chatWidget, &ChatWidget::messageSent,
            this, &MainWindow::sendChatMessage);
    connect(m_chatWidget, &ChatWidget::fileSelected,
            this, &MainWindow::sendFileMessage);
    connect(m_chatWidget, &ChatWidget::retryMessageRequested,
            this, &MainWindow::retryFailedMessage);
    connect(m_chatWidget, &ChatWidget::recallMessageRequested,
            this, &MainWindow::recallMessage);
    connect(m_chatWidget, &ChatWidget::messageListReachedBottom,
            this, &MainWindow::sendMarkRead);
    connect(m_chatWidget, &ChatWidget::fileMessageOpenRequested,
            this, &MainWindow::openFileMessage);

    connect(m_protocol, &ProtocolHandler::newMessageNotify,
            this, &MainWindow::onPushNewMessage);
    connect(m_protocol, &ProtocolHandler::recallNotify,
            this, &MainWindow::onPushRecall);
    connect(m_protocol, &ProtocolHandler::readReceiptNotify,
            this, &MainWindow::onPushReadReceipt);
    connect(m_protocol, &ProtocolHandler::friendRequestNotify,
            this, &MainWindow::onPushFriendRequest);
    connect(m_protocol, &ProtocolHandler::friendAcceptedNotify,
            this, &MainWindow::onPushFriendAccepted);
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

void MainWindow::loadFriends(const QString& groupId) {
    if (!m_protocol || !m_contactWidget) return;
    swift::zone::FriendGetFriendsPayload req;
    req.set_group_id(groupId.toStdString());
    std::string payload;
    if (!req.SerializeToString(&payload)) return;
    m_protocol->sendRequest("friend.get_friends",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this](int code, const QByteArray& data) {
        if (code != 0) return;
        swift::zone::FriendGetFriendsResponsePayload resp;
        if (!resp.ParseFromArray(data.data(), data.size())) return;
        QList<ContactWidget::FriendItem> friends;
        friends.reserve(resp.friends_size());
        for (const auto& f : resp.friends()) {
            ContactWidget::FriendItem item;
            item.friendId = QString::fromStdString(f.friend_id());
            item.nickname = QString::fromStdString(f.nickname());
            item.remark = QString::fromStdString(f.remark());
            item.groupId = QString::fromStdString(f.group_id());
            item.avatarUrl = QString::fromStdString(f.avatar_url());
            item.addedAt = f.added_at();
            friends.append(item);
        }
        m_contactWidget->setFriends(friends);
    });
}

void MainWindow::loadFriendRequests() {
    if (!m_protocol || !m_contactWidget) return;
    swift::zone::FriendGetRequestsPayload req;
    req.set_type(0);
    std::string payload;
    if (!req.SerializeToString(&payload)) return;
    m_protocol->sendRequest("friend.get_requests",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this](int code, const QByteArray& data) {
        if (code != 0) return;
        swift::zone::FriendGetRequestsResponsePayload resp;
        if (!resp.ParseFromArray(data.data(), data.size())) return;
        QList<ContactWidget::FriendRequestItem> requests;
        requests.reserve(resp.requests_size());
        for (const auto& r : resp.requests()) {
            ContactWidget::FriendRequestItem item;
            item.requestId = QString::fromStdString(r.request_id());
            item.fromUserId = QString::fromStdString(r.from_user_id());
            item.fromNickname = QString::fromStdString(r.from_nickname());
            item.fromAvatarUrl = QString::fromStdString(r.from_avatar_url());
            item.remark = QString::fromStdString(r.remark());
            item.status = r.status();
            item.createdAt = r.created_at();
            requests.append(item);
        }
        m_contactWidget->setFriendRequests(requests);
    });
}

void MainWindow::handleFriendRequest(const QString& requestId, bool accept) {
    if (!m_protocol || requestId.isEmpty()) return;
    swift::zone::FriendHandleRequestPayload req;
    req.set_user_id(m_currentUserId.toStdString());
    req.set_request_id(requestId.toStdString());
    req.set_accept(accept);
    std::string payload;
    if (!req.SerializeToString(&payload)) return;
    m_protocol->sendRequest("friend.handle_request",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this](int code, const QByteArray&, const QString& message) {
        if (code != 0) {
            const QString err = message.trimmed().isEmpty()
                ? QString("处理好友请求失败，错误码: %1").arg(code)
                : message;
            QMessageBox::warning(this, "操作失败", err);
            return;
        }
        loadFriendRequests();
        loadFriends();
    });
}

void MainWindow::loadUserGroups() {
    if (!m_protocol || !m_contactWidget) return;
    swift::zone::GroupGetUserGroupsPayload req;
    std::string payload;
    if (!req.SerializeToString(&payload)) return;
    m_protocol->sendRequest("group.get_user_groups",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this](int code, const QByteArray& data) {
        if (code != 0) return;
        swift::zone::GroupGetUserGroupsResponsePayload resp;
        if (!resp.ParseFromArray(data.data(), data.size())) return;
        QList<ContactWidget::GroupItem> groups;
        groups.reserve(resp.groups_size());
        for (const auto& g : resp.groups()) {
            ContactWidget::GroupItem item;
            item.groupId = QString::fromStdString(g.group_id());
            item.groupName = QString::fromStdString(g.group_name());
            item.avatarUrl = QString::fromStdString(g.avatar_url());
            item.ownerId = QString::fromStdString(g.owner_id());
            item.announcement = QString::fromStdString(g.announcement());
            item.memberCount = g.member_count();
            item.createdAt = g.created_at();
            groups.append(item);

            // 让群组在聊天主链路中可被选中和发消息。
            const QString key = convKey(item.groupId, 2);
            if (!m_conversationMap.contains(key)) {
                Conversation conv;
                conv.chatId = item.groupId;
                conv.chatType = 2;
                conv.peerId = item.groupId;
                conv.peerName = item.groupName;
                conv.peerAvatar = item.avatarUrl;
                conv.updatedAt = item.createdAt;
                m_conversationMap[key] = conv;
                m_contactWidget->upsertConversation(conv);
            }
        }
        m_contactWidget->setGroups(groups);
    });
}

void MainWindow::loadGroupInfo(const QString& groupId) {
    if (!m_protocol || !m_contactWidget || groupId.isEmpty()) return;
    swift::zone::GroupGetInfoPayload req;
    req.set_group_id(groupId.toStdString());
    std::string payload;
    if (!req.SerializeToString(&payload)) return;
    m_protocol->sendRequest("group.get_info",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this](int code, const QByteArray& data) {
        if (code != 0) return;
        swift::zone::GroupGetInfoResponsePayload resp;
        if (!resp.ParseFromArray(data.data(), data.size())) return;
        if (!resp.has_group()) return;
        const auto& g = resp.group();
        ContactWidget::GroupItem item;
        item.groupId = QString::fromStdString(g.group_id());
        item.groupName = QString::fromStdString(g.group_name());
        item.avatarUrl = QString::fromStdString(g.avatar_url());
        item.ownerId = QString::fromStdString(g.owner_id());
        item.announcement = QString::fromStdString(g.announcement());
        item.memberCount = g.member_count();
        item.createdAt = g.created_at();
        m_contactWidget->setCurrentGroupInfo(item);
    });
}

void MainWindow::loadGroupMembers(const QString& groupId) {
    if (!m_protocol || !m_contactWidget || groupId.isEmpty()) return;
    swift::zone::GroupGetMembersPayload req;
    req.set_group_id(groupId.toStdString());
    req.set_page(1);
    req.set_page_size(100);
    std::string payload;
    if (!req.SerializeToString(&payload)) return;
    m_protocol->sendRequest("group.get_members",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this](int code, const QByteArray& data) {
        if (code != 0) return;
        swift::zone::GroupGetMembersResponsePayload resp;
        if (!resp.ParseFromArray(data.data(), data.size())) return;
        QList<ContactWidget::GroupMemberItem> members;
        members.reserve(resp.members_size());
        for (const auto& m : resp.members()) {
            ContactWidget::GroupMemberItem item;
            item.userId = QString::fromStdString(m.user_id());
            item.nickname = QString::fromStdString(m.nickname());
            item.role = m.role();
            item.joinedAt = m.joined_at();
            members.append(item);
        }
        m_contactWidget->setGroupMembers(members);
    });
}

void MainWindow::createGroup() {
    if (!m_protocol) return;
    bool ok = false;
    const QString name = QInputDialog::getText(this, "创建群聊", "群名称：", QLineEdit::Normal, "", &ok).trimmed();
    if (!ok || name.isEmpty()) return;

    const QString membersInput = QInputDialog::getText(
        this, "创建群聊", "初始成员ID（逗号分隔，可留空）：", QLineEdit::Normal, "", &ok).trimmed();
    if (!ok) return;

    swift::zone::GroupCreatePayload req;
    req.set_creator_id(m_currentUserId.toStdString());
    req.set_group_name(name.toStdString());
    if (!membersInput.isEmpty()) {
        const QStringList ids = membersInput.split(',', Qt::SkipEmptyParts);
        for (const QString& raw : ids) {
            const QString id = raw.trimmed();
            if (!id.isEmpty() && id != m_currentUserId) {
                req.add_member_ids(id.toStdString());
            }
        }
    }

    std::string payload;
    if (!req.SerializeToString(&payload)) return;
    m_protocol->sendRequest("group.create",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this](int code, const QByteArray& data, const QString& message) {
        if (code != 0) {
            const QString err = message.trimmed().isEmpty()
                ? QString("创建群失败，错误码: %1").arg(code)
                : message;
            QMessageBox::warning(this, "创建群失败", err);
            return;
        }
        swift::zone::GroupCreateResponsePayload resp;
        if (resp.ParseFromArray(data.data(), data.size()) && !resp.success()) {
            const QString err = QString::fromStdString(resp.error()).trimmed();
            QMessageBox::warning(this, "创建群失败", err.isEmpty() ? QStringLiteral("服务端返回创建失败。") : err);
            return;
        }
        const QString groupId = resp.ParseFromArray(data.data(), data.size())
            ? QString::fromStdString(resp.group_id())
            : QString();
        loadUserGroups();
        if (!groupId.isEmpty()) {
            onConversationSelected(groupId, 2);
        }
    });
}

void MainWindow::inviteGroupMembers(const QString& groupId) {
    if (!m_protocol || groupId.isEmpty()) return;
    bool ok = false;
    const QString membersInput = QInputDialog::getText(
        this, "邀请成员", "成员ID（逗号分隔）：", QLineEdit::Normal, "", &ok).trimmed();
    if (!ok || membersInput.isEmpty()) return;

    swift::zone::GroupInviteMembersPayload req;
    req.set_group_id(groupId.toStdString());
    req.set_inviter_id(m_currentUserId.toStdString());
    const QStringList ids = membersInput.split(',', Qt::SkipEmptyParts);
    for (const QString& raw : ids) {
        const QString id = raw.trimmed();
        if (!id.isEmpty() && id != m_currentUserId) {
            req.add_member_ids(id.toStdString());
        }
    }

    std::string payload;
    if (!req.SerializeToString(&payload)) return;
    m_protocol->sendRequest("group.invite_members",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this, groupId](int code, const QByteArray&, const QString& message) {
        if (code != 0) {
            const QString err = message.trimmed().isEmpty()
                ? QString("邀请失败，错误码: %1").arg(code)
                : message;
            QMessageBox::warning(this, "邀请成员失败", err);
            return;
        }
        loadGroupMembers(groupId);
        loadGroupInfo(groupId);
    });
}

void MainWindow::removeGroupMember(const QString& groupId) {
    if (!m_protocol || groupId.isEmpty()) return;
    bool ok = false;
    const QString memberId = QInputDialog::getText(
        this, "踢出成员", "成员ID：", QLineEdit::Normal, "", &ok).trimmed();
    if (!ok || memberId.isEmpty()) return;

    swift::zone::GroupRemoveMemberPayload req;
    req.set_group_id(groupId.toStdString());
    req.set_operator_id(m_currentUserId.toStdString());
    req.set_member_id(memberId.toStdString());

    std::string payload;
    if (!req.SerializeToString(&payload)) return;
    m_protocol->sendRequest("group.remove_member",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this, groupId](int code, const QByteArray&, const QString& message) {
        if (code != 0) {
            const QString err = message.trimmed().isEmpty()
                ? QString("踢人失败，错误码: %1").arg(code)
                : message;
            QMessageBox::warning(this, "踢人失败", err);
            return;
        }
        loadGroupMembers(groupId);
        loadGroupInfo(groupId);
    });
}

void MainWindow::leaveGroup(const QString& groupId) {
    if (!m_protocol || groupId.isEmpty()) return;
    const auto reply = QMessageBox::question(this, "退群", QString("确定退出群 %1 吗？").arg(groupId));
    if (reply != QMessageBox::Yes) return;

    swift::zone::GroupLeavePayload req;
    req.set_group_id(groupId.toStdString());
    req.set_user_id(m_currentUserId.toStdString());
    std::string payload;
    if (!req.SerializeToString(&payload)) return;
    m_protocol->sendRequest("group.leave",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this, groupId](int code, const QByteArray&, const QString& message) {
        if (code != 0) {
            const QString err = message.trimmed().isEmpty()
                ? QString("退群失败，错误码: %1").arg(code)
                : message;
            QMessageBox::warning(this, "退群失败", err);
            return;
        }
        const QString key = convKey(groupId, 2);
        m_conversationMap.remove(key);
        m_messageMap.remove(key);
        if (m_currentChatType == 2 && m_currentChatId == groupId) {
            m_currentChatId.clear();
            if (m_chatWidget) {
                m_chatWidget->setConversation(QString(), 2);
            }
        }
        loadUserGroups();
    });
}

void MainWindow::loadHistory(const QString& chatId, int chatType) {
    if (!m_appService) return;
    m_appService->sendChatGetHistory(chatId, chatType, QString(), 50,
                                     [this, chatId, chatType](int code, const QString&, const swift::zone::ChatGetHistoryResponsePayload& resp) {
        if (code != 0) return;

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

void MainWindow::pullOfflineMessages(int limit) {
    if (!m_protocol || m_offlinePullInFlight) return;
    swift::zone::ChatPullOfflinePayload req;
    req.set_limit(limit > 0 ? limit : 100);
    if (!m_offlineCursor.isEmpty()) {
        req.set_cursor(m_offlineCursor.toStdString());
    }
    std::string payload;
    if (!req.SerializeToString(&payload)) return;

    m_offlinePullInFlight = true;
    m_protocol->sendRequest("chat.pull_offline",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this](int code, const QByteArray& data) {
        m_offlinePullInFlight = false;
        if (code != 0) return;

        swift::zone::ChatPullOfflineResponsePayload resp;
        if (!resp.ParseFromArray(data.data(), data.size())) return;
        m_offlineCursor = QString::fromStdString(resp.next_cursor());

        bool currentConversationTouched = false;
        const QString currentKey = convKey(m_currentChatId, m_currentChatType);
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

            if (mergeOfflineMessage(msg)) {
                if (convKey(msg.toId, msg.chatType) == currentKey) {
                    currentConversationTouched = true;
                }
            }
        }

        if (currentConversationTouched && m_messageMap.contains(currentKey)) {
            m_chatWidget->setMessages(m_messageMap.value(currentKey));
            sendMarkRead();
        }
    });
}

bool MainWindow::mergeOfflineMessage(const Message& msg) {
    const QString key = convKey(msg.toId, msg.chatType);
    auto& list = m_messageMap[key];
    if (!msg.msgId.isEmpty()) {
        for (const auto& existed : list) {
            if (existed.msgId == msg.msgId) {
                return false;
            }
        }
    }
    list.append(msg);

    Conversation conv = m_conversationMap.value(key);
    conv.chatId = msg.toId;
    conv.chatType = msg.chatType;
    if (conv.peerName.isEmpty()) conv.peerName = msg.toId;
    if (msg.timestamp >= conv.updatedAt) {
        conv.lastMessage = msg;
        conv.updatedAt = msg.timestamp;
    }
    if (!(msg.toId == m_currentChatId && msg.chatType == m_currentChatType) && !msg.isSelf) {
        conv.unreadCount += 1;
    }
    m_conversationMap[key] = conv;
    m_contactWidget->upsertConversation(conv);
    return true;
}

void MainWindow::sendChatMessage(const QString& content) {
    if (!m_protocol || m_currentChatId.isEmpty() || content.isEmpty()) return;

    const QString chatId = m_currentChatId;
    const int chatType = m_currentChatType;
    const QString key = convKey(chatId, chatType);
    const QString localMsgId = QString("local_%1").arg(QDateTime::currentMSecsSinceEpoch());
    const qint64 localTs = QDateTime::currentMSecsSinceEpoch();

    Message local;
    local.msgId = localMsgId;
    local.fromUserId = m_currentUserId;
    local.toId = chatId;
    local.chatType = chatType;
    local.content = content;
    local.mediaType = "text";
    local.timestamp = localTs;
    local.isSelf = true;
    local.sending = true;
    local.sendFailed = false;
    m_messageMap[key].append(local);
    if (m_chatWidget->chatId() == chatId && m_chatWidget->chatType() == chatType) {
        m_chatWidget->appendMessage(local);
    }

    swift::zone::ChatSendMessagePayload req;
    req.set_from_user_id(m_currentUserId.toStdString());
    req.set_to_id(chatId.toStdString());
    req.set_chat_type(chatType);
    req.set_content(content.toStdString());
    req.set_media_type("text");
    req.set_client_msg_id(localMsgId.toStdString());

    std::string payload;
    if (!req.SerializeToString(&payload)) return;
    m_protocol->sendRequest("chat.send_message",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this, key, localMsgId](int code, const QByteArray& data, const QString& message) {
        auto& messages = m_messageMap[key];
        int idx = -1;
        for (int i = 0; i < messages.size(); ++i) {
            if (messages[i].msgId == localMsgId) {
                idx = i;
                break;
            }
        }
        if (idx < 0) return;

        if (code != 0) {
            messages[idx].sending = false;
            messages[idx].sendFailed = true;
            if (m_chatWidget->chatId() == messages[idx].toId && m_chatWidget->chatType() == messages[idx].chatType) {
                m_chatWidget->setMessages(messages);
            }
            const QString err = message.trimmed().isEmpty()
                ? QString("发送失败，错误码: %1").arg(code)
                : message;
            QMessageBox::warning(this, "消息发送失败", err);
            return;
        }
        swift::zone::ChatSendMessageResponsePayload resp;
        if (!resp.ParseFromArray(data.data(), data.size()) || !resp.success()) {
            messages[idx].sending = false;
            messages[idx].sendFailed = true;
            if (m_chatWidget->chatId() == messages[idx].toId && m_chatWidget->chatType() == messages[idx].chatType) {
                m_chatWidget->setMessages(messages);
            }
            const QString err = QString::fromStdString(resp.error()).trimmed();
            QMessageBox::warning(this, "消息发送失败", err.isEmpty() ? "服务端未返回成功结果。" : err);
            return;
        }

        messages[idx].sending = false;
        messages[idx].sendFailed = false;
        if (!resp.msg_id().empty()) {
            messages[idx].msgId = QString::fromStdString(resp.msg_id());
        }
        if (resp.timestamp() > 0) {
            messages[idx].timestamp = resp.timestamp();
        }

        const Message& msg = messages[idx];
        if (m_chatWidget->chatId() == msg.toId && m_chatWidget->chatType() == msg.chatType) {
            m_chatWidget->setMessages(messages);
        }
        auto convIt = m_conversationMap.find(key);
        if (convIt != m_conversationMap.end()) {
            convIt->lastMessage = msg;
            convIt->updatedAt = msg.timestamp;
            m_contactWidget->upsertConversation(*convIt);
        }
    });
}

void MainWindow::retryFailedMessage(const QString& msgId) {
    if (!m_protocol || m_currentChatId.isEmpty() || msgId.isEmpty()) return;
    const QString key = convKey(m_currentChatId, m_currentChatType);
    auto& messages = m_messageMap[key];
    int idx = -1;
    for (int i = 0; i < messages.size(); ++i) {
        if (messages[i].msgId == msgId) {
            idx = i;
            break;
        }
    }
    if (idx < 0 || !messages[idx].sendFailed) return;

    Message& failed = messages[idx];
    if (failed.mediaType.compare("text", Qt::CaseInsensitive) != 0) {
        QMessageBox::information(this, "暂不支持重发", "当前仅支持重发文本消息。");
        return;
    }

    failed.sending = true;
    failed.sendFailed = false;
    if (m_chatWidget->chatId() == failed.toId && m_chatWidget->chatType() == failed.chatType) {
        m_chatWidget->setMessages(messages);
    }

    swift::zone::ChatSendMessagePayload req;
    req.set_from_user_id(m_currentUserId.toStdString());
    req.set_to_id(failed.toId.toStdString());
    req.set_chat_type(failed.chatType);
    req.set_content(failed.content.toStdString());
    req.set_media_type("text");
    req.set_client_msg_id(failed.msgId.toStdString());

    std::string payload;
    if (!req.SerializeToString(&payload)) {
        failed.sending = false;
        failed.sendFailed = true;
        if (m_chatWidget->chatId() == failed.toId && m_chatWidget->chatType() == failed.chatType) {
            m_chatWidget->setMessages(messages);
        }
        return;
    }

    const QString retryMsgId = failed.msgId;
    m_protocol->sendRequest("chat.send_message",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this, key, retryMsgId](int code, const QByteArray& data, const QString& message) {
        auto& msgs = m_messageMap[key];
        int retryIdx = -1;
        for (int i = 0; i < msgs.size(); ++i) {
            if (msgs[i].msgId == retryMsgId) {
                retryIdx = i;
                break;
            }
        }
        if (retryIdx < 0) return;

        if (code != 0) {
            msgs[retryIdx].sending = false;
            msgs[retryIdx].sendFailed = true;
            if (m_chatWidget->chatId() == msgs[retryIdx].toId && m_chatWidget->chatType() == msgs[retryIdx].chatType) {
                m_chatWidget->setMessages(msgs);
            }
            const QString err = message.trimmed().isEmpty()
                ? QString("重发失败，错误码: %1").arg(code)
                : message;
            QMessageBox::warning(this, "消息重发失败", err);
            return;
        }

        swift::zone::ChatSendMessageResponsePayload resp;
        if (!resp.ParseFromArray(data.data(), data.size()) || !resp.success()) {
            msgs[retryIdx].sending = false;
            msgs[retryIdx].sendFailed = true;
            if (m_chatWidget->chatId() == msgs[retryIdx].toId && m_chatWidget->chatType() == msgs[retryIdx].chatType) {
                m_chatWidget->setMessages(msgs);
            }
            const QString err = QString::fromStdString(resp.error()).trimmed();
            QMessageBox::warning(this, "消息重发失败", err.isEmpty() ? "服务端未返回成功结果。" : err);
            return;
        }

        msgs[retryIdx].sending = false;
        msgs[retryIdx].sendFailed = false;
        if (!resp.msg_id().empty()) {
            msgs[retryIdx].msgId = QString::fromStdString(resp.msg_id());
        }
        if (resp.timestamp() > 0) {
            msgs[retryIdx].timestamp = resp.timestamp();
        }

        if (m_chatWidget->chatId() == msgs[retryIdx].toId && m_chatWidget->chatType() == msgs[retryIdx].chatType) {
            m_chatWidget->setMessages(msgs);
        }
    });
}

void MainWindow::sendFileMessage(const QString& filePath) {
    if (!m_protocol || m_currentChatId.isEmpty() || filePath.isEmpty()) return;
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        QMessageBox::warning(this, "文件发送失败", "选择的文件不存在或不可用。");
        return;
    }

    swift::zone::FileGetUploadTokenPayload tokenReq;
    tokenReq.set_user_id(m_currentUserId.toStdString());
    tokenReq.set_file_name(fileInfo.fileName().toStdString());
    tokenReq.set_file_size(fileInfo.size());

    std::string tokenPayload;
    if (!tokenReq.SerializeToString(&tokenPayload)) return;
    m_protocol->sendRequest("file.get_upload_token",
                            QByteArray(tokenPayload.data(), static_cast<int>(tokenPayload.size())),
                            [this, filePath, fileInfo](int code, const QByteArray& data, const QString& message) {
        if (code != 0) {
            const QString err = message.trimmed().isEmpty()
                ? QString("获取上传令牌失败，错误码: %1").arg(code)
                : message;
            QMessageBox::warning(this, "文件发送失败", err);
            return;
        }

        swift::zone::FileGetUploadTokenResponsePayload tokenResp;
        if (!tokenResp.ParseFromArray(data.data(), data.size()) || !tokenResp.success()) {
            const QString err = QString::fromStdString(tokenResp.error()).trimmed();
            QMessageBox::warning(this, "文件发送失败", err.isEmpty() ? "上传令牌响应无效。" : err);
            return;
        }

        const QString uploadUrl = QString::fromStdString(tokenResp.upload_url());
        const QString uploadToken = QString::fromStdString(tokenResp.upload_token());
        QString mediaUrl;
        QString uploadError;
        if (!uploadFileByHttp(uploadUrl, uploadToken, filePath, &mediaUrl, &uploadError)) {
            QMessageBox::warning(this, "文件发送失败", uploadError.isEmpty() ? "文件上传失败。" : uploadError);
            return;
        }

        swift::zone::ChatSendMessagePayload req;
        req.set_from_user_id(m_currentUserId.toStdString());
        req.set_to_id(m_currentChatId.toStdString());
        req.set_chat_type(m_currentChatType);
        req.set_content(fileInfo.fileName().toStdString());
        req.set_media_type("file");
        req.set_media_url(mediaUrl.toStdString());
        req.set_file_size(fileInfo.size());
        req.set_client_msg_id(QString("c_file_%1").arg(QDateTime::currentMSecsSinceEpoch()).toStdString());

        std::string payload;
        if (!req.SerializeToString(&payload)) return;
        m_protocol->sendRequest("chat.send_message",
                                QByteArray(payload.data(), static_cast<int>(payload.size())),
                                [this, mediaUrl, fileInfo](int sendCode, const QByteArray& sendData, const QString& sendMessage) {
            if (sendCode != 0) {
                const QString err = sendMessage.trimmed().isEmpty()
                    ? QString("发送文件消息失败，错误码: %1").arg(sendCode)
                    : sendMessage;
                QMessageBox::warning(this, "文件发送失败", err);
                return;
            }
            swift::zone::ChatSendMessageResponsePayload resp;
            if (!resp.ParseFromArray(sendData.data(), sendData.size()) || !resp.success()) {
                QMessageBox::warning(this, "文件发送失败", "服务端未返回成功结果。");
                return;
            }

            Message msg;
            msg.msgId = QString::fromStdString(resp.msg_id());
            msg.fromUserId = m_currentUserId;
            msg.toId = m_currentChatId;
            msg.chatType = m_currentChatType;
            msg.content = fileInfo.fileName();
            msg.mediaType = "file";
            msg.mediaUrl = mediaUrl;
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
    });
}

bool MainWindow::uploadFileByHttp(const QString& uploadUrl,
                                  const QString& uploadToken,
                                  const QString& filePath,
                                  QString* mediaUrl,
                                  QString* error) {
    if (!m_networkManager || uploadUrl.trimmed().isEmpty() || uploadToken.trimmed().isEmpty()) {
        if (error) *error = "上传参数无效。";
        return false;
    }
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = "读取本地文件失败。";
        return false;
    }
    const QByteArray body = file.readAll();
    file.close();

    QNetworkRequest request{QUrl(uploadUrl)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(uploadToken).toUtf8());
    request.setRawHeader("X-Upload-Token", uploadToken.toUtf8());
    request.setRawHeader("X-File-Name", QFileInfo(filePath).fileName().toUtf8());

    QNetworkReply* reply = m_networkManager->put(request, body);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const QByteArray respBody = reply->readAll();
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const bool ok = (reply->error() == QNetworkReply::NoError) && (status >= 200 && status < 300);
    if (!ok) {
        if (error) {
            const QString detail = QString::fromUtf8(respBody).trimmed();
            *error = detail.isEmpty() ? reply->errorString() : detail;
        }
        reply->deleteLater();
        return false;
    }

    QString resolvedUrl;
    const QJsonDocument doc = QJsonDocument::fromJson(respBody);
    if (doc.isObject()) {
        const QJsonObject obj = doc.object();
        resolvedUrl = obj.value("file_url").toString();
        if (resolvedUrl.isEmpty()) resolvedUrl = obj.value("url").toString();
        if (resolvedUrl.isEmpty()) resolvedUrl = obj.value("media_url").toString();
    }
    if (resolvedUrl.isEmpty()) {
        const QVariant location = reply->header(QNetworkRequest::LocationHeader);
        if (location.isValid()) {
            resolvedUrl = location.toUrl().toString();
        }
    }
    if (resolvedUrl.isEmpty()) {
        resolvedUrl = uploadToken;
    }
    if (mediaUrl) *mediaUrl = resolvedUrl;
    reply->deleteLater();
    return true;
}

bool MainWindow::resolveFileDownloadUrl(const Message& msg, QString* fileUrl, QString* fileName, QString* error) {
    if (fileName) {
        *fileName = msg.content.trimmed().isEmpty() ? QStringLiteral("download.bin") : msg.content.trimmed();
    }
    const QString media = msg.mediaUrl.trimmed();
    if (media.startsWith("http://") || media.startsWith("https://")) {
        if (fileUrl) *fileUrl = media;
        return true;
    }
    if (media.isEmpty()) {
        if (error) *error = "文件标识为空。";
        return false;
    }

    swift::zone::FileGetFileUrlPayload req;
    req.set_file_id(media.toStdString());
    req.set_user_id(m_currentUserId.toStdString());
    std::string payload;
    if (!req.SerializeToString(&payload)) {
        if (error) *error = "构造下载请求失败。";
        return false;
    }

    bool finished = false;
    bool ok = false;
    QString resolved;
    QString resolvedName;
    QString err;
    m_protocol->sendRequest("file.get_file_url",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this, &finished, &ok, &resolved, &resolvedName, &err](int code, const QByteArray& data, const QString& message) {
        finished = true;
        if (code != 0) {
            err = message.trimmed().isEmpty()
                ? QString("获取下载地址失败，错误码: %1").arg(code)
                : message;
            return;
        }
        swift::zone::FileGetFileUrlResponsePayload resp;
        if (!resp.ParseFromArray(data.data(), data.size()) || !resp.success()) {
            const QString detail = QString::fromStdString(resp.error()).trimmed();
            err = detail.isEmpty() ? QStringLiteral("下载地址响应无效。") : detail;
            return;
        }
        resolved = QString::fromStdString(resp.file_url());
        resolvedName = QString::fromStdString(resp.file_name());
        ok = !resolved.isEmpty();
        if (!ok) {
            err = "未返回可用下载地址。";
        }
    });

    QElapsedTimer timer;
    timer.start();
    while (!finished && timer.elapsed() < 16000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }
    if (!finished) {
        if (error) *error = "获取下载地址超时。";
        return false;
    }
    if (!ok) {
        if (error) *error = err;
        return false;
    }
    if (fileUrl) *fileUrl = resolved;
    if (fileName && !resolvedName.trimmed().isEmpty()) *fileName = resolvedName.trimmed();
    return true;
}

bool MainWindow::downloadFileToPath(const QString& fileUrl, const QString& savePath, QString* error) {
    if (!m_networkManager || fileUrl.trimmed().isEmpty() || savePath.trimmed().isEmpty()) {
        if (error) *error = "下载参数无效。";
        return false;
    }
    QNetworkRequest request{QUrl(fileUrl)};
    request.setRawHeader("Range", "bytes=0-");
    QNetworkReply* reply = m_networkManager->get(request);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const QByteArray body = reply->readAll();
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const bool ok = (reply->error() == QNetworkReply::NoError) && (status == 200 || status == 206);
    if (!ok) {
        if (error) *error = reply->errorString();
        reply->deleteLater();
        return false;
    }

    QSaveFile out(savePath);
    if (!out.open(QIODevice::WriteOnly)) {
        if (error) *error = "写入目标文件失败。";
        reply->deleteLater();
        return false;
    }
    out.write(body);
    if (!out.commit()) {
        if (error) *error = "保存下载文件失败。";
        reply->deleteLater();
        return false;
    }
    reply->deleteLater();
    return true;
}

void MainWindow::openFileMessage(const QString& msgId) {
    if (msgId.isEmpty()) return;
    const QString key = convKey(m_currentChatId, m_currentChatType);
    const auto messages = m_messageMap.value(key);
    Message hit;
    bool found = false;
    for (const auto& m : messages) {
        if (m.msgId == msgId) {
            hit = m;
            found = true;
            break;
        }
    }
    if (!found || hit.mediaType.compare("file", Qt::CaseInsensitive) != 0) return;

    QString url;
    QString suggestName;
    QString err;
    if (!resolveFileDownloadUrl(hit, &url, &suggestName, &err)) {
        QMessageBox::warning(this, "下载失败", err.isEmpty() ? "无法获取下载地址。" : err);
        return;
    }
    const QString savePath = QFileDialog::getSaveFileName(this, "保存文件", suggestName);
    if (savePath.isEmpty()) return;
    if (!downloadFileToPath(url, savePath, &err)) {
        QMessageBox::warning(this, "下载失败", err.isEmpty() ? "文件下载失败。" : err);
        return;
    }
    QMessageBox::information(this, "下载完成", QString("已保存到：\n%1").arg(savePath));
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
                            ProtocolHandler::ResponseCallback());
}

void MainWindow::recallMessage(const QString& msgId) {
    if (!m_protocol || msgId.isEmpty()) return;
    const QString key = convKey(m_currentChatId, m_currentChatType);
    auto& msgs = m_messageMap[key];

    int idx = -1;
    for (int i = 0; i < msgs.size(); ++i) {
        if (msgs[i].msgId == msgId) {
            idx = i;
            break;
        }
    }
    if (idx < 0) return;
    if (!msgs[idx].isSelf || msgs[idx].status == 1 || msgs[idx].sending || msgs[idx].sendFailed) {
        return;
    }

    swift::zone::ChatRecallMessagePayload req;
    req.set_msg_id(msgId.toStdString());
    req.set_user_id(m_currentUserId.toStdString());
    std::string payload;
    if (!req.SerializeToString(&payload)) return;

    m_protocol->sendRequest("chat.recall_message",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this, key, msgId](int code, const QByteArray&, const QString& message) {
        if (code != 0) {
            const QString err = message.trimmed().isEmpty()
                ? QString("撤回失败，错误码: %1").arg(code)
                : message;
            QMessageBox::warning(this, "撤回失败", err);
            return;
        }

        auto& localMsgs = m_messageMap[key];
        for (auto& m : localMsgs) {
            if (m.msgId == msgId) {
                m.status = 1;
                break;
            }
        }
        if (m_chatWidget->chatId() == m_currentChatId && m_chatWidget->chatType() == m_currentChatType) {
            m_chatWidget->markMessageRecalled(msgId);
        }

        auto convIt = m_conversationMap.find(key);
        if (convIt != m_conversationMap.end()) {
            for (int i = localMsgs.size() - 1; i >= 0; --i) {
                if (!localMsgs[i].msgId.isEmpty()) {
                    convIt->lastMessage = localMsgs[i];
                    convIt->updatedAt = localMsgs[i].timestamp;
                    break;
                }
            }
            m_contactWidget->upsertConversation(*convIt);
        }
    });
}

QString MainWindow::convKey(const QString& chatId, int chatType) const {
    return QString("%1#%2").arg(chatType).arg(chatId);
}

void MainWindow::onConversationSelected(const QString& chatId, int chatType) {
    if (m_rightStack && m_chatWidget) {
        m_rightStack->setCurrentWidget(m_chatWidget);
    }
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
        if (!m_offlineCursor.isEmpty()) {
            pullOfflineMessages();
        }
        return;
    }
    loadHistory(chatId, chatType);
    if (!m_offlineCursor.isEmpty()) {
        pullOfflineMessages();
    }
}

void MainWindow::onFriendSelected(const QString& userId) {
    if (!m_contactWidget || !m_rightStack || !m_friendProfilePage) return;
    const ContactWidget::FriendItem f = m_contactWidget->friendById(userId);
    m_profileUserId = f.friendId.isEmpty() ? userId : f.friendId;
    m_profileNickname = f.nickname;
    m_profileRemark = f.remark;
    m_profileGroupId = f.groupId;
    m_profileAvatarUrl = f.avatarUrl;
    m_profileAddedAt = f.addedAt;
    refreshFriendProfileCard();
    m_rightStack->setCurrentWidget(m_friendProfilePage);
}

void MainWindow::onGroupSelected(const QString& groupId) {
    if (groupId.isEmpty()) return;
    if (m_rightStack && m_chatWidget) {
        m_rightStack->setCurrentWidget(m_chatWidget);
    }
    loadGroupInfo(groupId);
    loadGroupMembers(groupId);
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

void MainWindow::onPushFriendRequest(const QByteArray& payload) {
    QString prompt = QStringLiteral("收到新的好友申请。");
    swift::gate::FriendRequestNotify notify;
    if (notify.ParseFromArray(payload.data(), payload.size())) {
        const QString fromUserId = QString::fromStdString(notify.from_user_id());
        const QString fromNickname = QString::fromStdString(notify.from_nickname());
        const QString showName = fromNickname.isEmpty() ? fromUserId : fromNickname;
        if (!showName.isEmpty()) {
            prompt = QString("%1 请求加你为好友。").arg(showName);
        }
    }
    QMessageBox::information(this, "好友申请", prompt);
    loadFriendRequests();
}

void MainWindow::onPushFriendAccepted(const QByteArray& payload) {
    QString prompt = QStringLiteral("有新的好友已通过你的请求。");
    swift::gate::FriendAcceptedNotify notify;
    if (notify.ParseFromArray(payload.data(), payload.size())) {
        const QString userId = QString::fromStdString(notify.friend_id());
        const QString nickname = QString::fromStdString(notify.nickname());
        const QString showName = nickname.isEmpty() ? userId : nickname;
        if (!showName.isEmpty()) {
            prompt = QString("%1 已成为你的好友。").arg(showName);
        }
    }
    QMessageBox::information(this, "好友通知", prompt);
    loadFriends();
    loadFriendRequests();
}

void MainWindow::refreshFriendProfileCard() {
    if (m_profileUserId.isEmpty()) return;
    const QString showName = m_profileRemark.isEmpty()
        ? (m_profileNickname.isEmpty() ? m_profileUserId : m_profileNickname)
        : m_profileRemark;
    if (m_profileNameLabel) m_profileNameLabel->setText(showName);
    if (m_profileIdLabel) m_profileIdLabel->setText(QString("ID: %1").arg(m_profileUserId));
    if (m_profileRemarkLabel) {
        m_profileRemarkLabel->setText(QString("备注: %1").arg(m_profileRemark.isEmpty() ? "无" : m_profileRemark));
    }
    if (m_profileSourceLabel) m_profileSourceLabel->setText("来源: 好友列表同步");
    if (m_profileAddedAtLabel) {
        const QString added = m_profileAddedAt > 0
            ? QDateTime::fromMSecsSinceEpoch(m_profileAddedAt).toString("yyyy-MM-dd HH:mm")
            : QString("未知");
        m_profileAddedAtLabel->setText(QString("加好友时间: %1").arg(added));
    }
    if (m_profileTagStarLabel) {
        m_profileTagStarLabel->setText(m_starFriendIds.contains(m_profileUserId) ? "星标好友" : "普通好友");
    }
    if (m_profileTagGroupLabel) {
        m_profileTagGroupLabel->setText(QString("分组: %1").arg(m_profileGroupId.isEmpty() ? "默认" : m_profileGroupId));
    }
    if (m_profileAvatarLabel) {
        QPixmap pix(m_profileAvatarUrl);
        if (!pix.isNull()) {
            const QPixmap circle = ImageUtils::makeCircular(pix).scaled(68, 68, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            m_profileAvatarLabel->setPixmap(circle);
            m_profileAvatarLabel->setText("");
        } else {
            m_profileAvatarLabel->setPixmap(QPixmap());
            m_profileAvatarLabel->setText(showName.left(1).toUpper());
        }
    }
}

void MainWindow::removeCurrentFriend() {
    if (!m_protocol || m_profileUserId.isEmpty()) return;
    const auto reply = QMessageBox::question(this, "删除好友", QString("确定删除好友 %1 吗？").arg(m_profileUserId));
    if (reply != QMessageBox::Yes) return;
    swift::zone::FriendRemovePayload req;
    req.set_user_id(m_currentUserId.toStdString());
    req.set_friend_id(m_profileUserId.toStdString());
    std::string payload;
    if (!req.SerializeToString(&payload)) return;
    m_protocol->sendRequest("friend.remove",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this](int code, const QByteArray&) {
        if (code != 0) {
            QMessageBox::warning(this, "删除失败", QString("删除好友失败，错误码: %1").arg(code));
            return;
        }
        if (m_contactWidget) m_contactWidget->removeFriendById(m_profileUserId);
        m_starFriendIds.remove(m_profileUserId);
        m_profileUserId.clear();
        if (m_rightStack && m_chatWidget) {
            m_rightStack->setCurrentWidget(m_chatWidget);
        }
    });
}
