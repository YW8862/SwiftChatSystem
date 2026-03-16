#include "mainwindow.h"

#include "contactwidget.h"
#include "chatwidget.h"
#include "blacklistdialog.h"
#include "network/app_service.h"
#include "network/protocol_handler.h"
#include "network/websocket_client.h"
#include "zone.pb.h"
#include "gate.pb.h"
#include "utils/settings.h"
#include "utils/chat_storage.h"
#include "swift/log_helper.h"

#include <QDateTime>
#include <functional>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QList>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QPointer>
#include <QSaveFile>
#include <QStackedWidget>
#include <QSplitter>
#include <QStyle>
#include <QToolButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <QEventLoop>
#include <QElapsedTimer>
#include <QTimer>
#include <QFileDialog>
#include <QFile>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <algorithm>
#include <QUrl>
#include "utils/image_utils.h"

MainWindow::MainWindow(ProtocolHandler* protocol,
                       WebSocketClient* wsClient,
                       const QString& currentUserId,
                       QWidget *parent)
    : QMainWindow(parent)
    , m_protocol(protocol)
    , m_wsClient(wsClient)
    , m_currentUserId(currentUserId) {
    setWindowTitle("SwiftChat");
    resize(1024, 768);

    setupUi();
    m_networkManager = new QNetworkAccessManager(this);
    m_appService = std::make_unique<client::AppService>(m_protocol);
    wireSignals();
    loadLocalConversationsAndMessages();
    triggerSessionValidation();
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

    auto* quickActionBtn = new QToolButton(navPanel);
    quickActionBtn->setObjectName("navPlusBtn");
    quickActionBtn->setText("+");
    quickActionBtn->setCursor(Qt::PointingHandCursor);
    quickActionBtn->setFixedSize(40, 40);
    quickActionBtn->setPopupMode(QToolButton::InstantPopup);
    auto* quickMenu = new QMenu(quickActionBtn);
    quickMenu->setStyleSheet(
        "QMenu{background:#ffffff;border:1px solid #dfe4ec;border-radius:10px;padding:6px 0;}"
        "QMenu::item{padding:8px 16px;color:#2f3440;font-size:13px;min-width:120px;}"
        "QMenu::item:selected{background:#ecf3ff;color:#2c63db;}"
    );
    QAction* addFriendAction = quickMenu->addAction("添加好友");
    QAction* createGroupAction = quickMenu->addAction("创建群聊");
    quickMenu->addSeparator();
    QAction* blacklistAction = quickMenu->addAction("黑名单管理");
    quickActionBtn->setMenu(quickMenu);
    navLayout->addWidget(quickActionBtn, 0, Qt::AlignHCenter);
    navLayout->addStretch();

    m_contactWidget = new ContactWidget(central);
    m_contactWidget->setCurrentUserId(m_currentUserId);
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
    connect(addFriendAction, &QAction::triggered, this, [this]() {
        showAddFriendDialog();
    });
    connect(createGroupAction, &QAction::triggered, this, [this]() {
        createGroup();
    });
    connect(blacklistAction, &QAction::triggered, this, [this]() {
        loadBlacklist();  // 先加载黑名单
        showBlacklistDialog();
    });
    connect(m_profileSendMsgBtn, &QPushButton::clicked, this, [this]() {
        if (m_profileUserId.isEmpty()) return;
        const QString key = convKey(m_profileUserId, 1);
        // 先检查会话是否已存在
        if (!m_conversationMap.contains(key)) {
            // 会话不存在时才创建
            Conversation conv;
            conv.chatId = m_profileUserId;
            conv.chatType = 1;
            conv.peerId = m_profileUserId;
            conv.peerName = m_profileNickname.isEmpty() ? m_profileUserId : m_profileNickname;
            conv.updatedAt = QDateTime::currentMSecsSinceEpoch();
            m_conversationMap[key] = conv;
            m_contactWidget->upsertConversation(conv);
        }
        // 切换到会话列表并打开该会话
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
        "QToolButton#navPlusBtn { background: transparent; border: none; border-radius: 20px; color: #d5d5d5; font-size: 22px; font-weight: 500; }"
        "QToolButton#navPlusBtn:hover { background: #3c3d42; color: #ffffff; }"
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
    connect(m_chatWidget, &ChatWidget::conversationMoreRequested,
            this, &MainWindow::onConversationMoreRequested);

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
    if (m_wsClient) {
        connect(m_wsClient, &WebSocketClient::disconnected, this, [this]() {
            m_sessionReady = false;
            m_sessionValidationInFlight = false;
            LogWarning("[MainWindow] websocket disconnected, session marked not ready");
        });
        connect(m_wsClient, &WebSocketClient::connected, this, [this]() {
            m_sessionReady = false;
            LogInfo("[MainWindow] websocket connected, schedule session validation after 300ms");
            QTimer::singleShot(300, this, [this]() {
                if (m_wsClient && m_wsClient->isConnected()) {
                    LogInfo("[MainWindow] deferred trigger session validation");
                    triggerSessionValidation();
                }
            });
        });
    }
}

void MainWindow::loadLocalConversationsAndMessages() {
    if (m_currentUserId.trimmed().isEmpty()) return;
    QList<Conversation> convs = ChatStorage::instance().loadConversations(m_currentUserId);
    if (convs.isEmpty()) return;
    m_conversationMap.clear();
    for (const auto& c : convs) {
        m_conversationMap[convKey(c.chatId, c.chatType)] = c;
    }
    m_contactWidget->setConversations(convs);
}

void MainWindow::saveConversationsToLocal() {
    if (m_currentUserId.trimmed().isEmpty()) return;
    ChatStorage::instance().mergeAndSaveConversations(m_currentUserId, m_conversationMap);
}

void MainWindow::saveMessagesToLocal(const QString& chatId, int chatType) {
    if (m_currentUserId.trimmed().isEmpty() || chatId.isEmpty()) return;
    const QString key = convKey(chatId, chatType);
    if (!m_messageMap.contains(key)) return;
    ChatStorage::instance().mergeAndSaveMessages(m_currentUserId, chatId, chatType,
                                                  m_messageMap[key], 500);
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
        saveConversationsToLocal();
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
            item.toUserId = QString::fromStdString(r.to_user_id());
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
    if (!m_protocol || !m_contactWidget) return;
    const QList<ContactWidget::FriendItem> friends = m_contactWidget->friends();
    if (friends.isEmpty()) {
        QMessageBox::information(this, "创建群聊", "当前没有可选好友，请先添加好友。");
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("创建群聊");
    dialog.resize(440, 520);
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(16, 16, 16, 12);
    layout->setSpacing(10);

    auto* nameEdit = new QLineEdit(&dialog);
    nameEdit->setPlaceholderText("请输入群聊名称");
    nameEdit->setClearButtonEnabled(true);
    nameEdit->setMinimumHeight(34);
    layout->addWidget(nameEdit);

    auto* tip = new QLabel("选择要加入群聊的好友", &dialog);
    tip->setStyleSheet("color:#7f8694;font-size:12px;");
    layout->addWidget(tip);

    auto* list = new QListWidget(&dialog);
    list->setSelectionMode(QAbstractItemView::NoSelection);
    list->setStyleSheet(
        "QListWidget{background:#fff;border:1px solid #e1e6ef;border-radius:10px;}"
        "QListWidget::item{height:36px;}"
    );
    for (const auto& f : friends) {
        const QString userId = f.friendId.trimmed();
        if (userId.isEmpty()) continue;
        const QString display = f.remark.trimmed().isEmpty()
            ? (f.nickname.trimmed().isEmpty() ? userId : f.nickname.trimmed())
            : f.remark.trimmed();
        auto* item = new QListWidgetItem(list);
        auto* cb = new QCheckBox(display, list);
        cb->setProperty("userId", userId);
        list->setItemWidget(item, cb);
    }
    layout->addWidget(list, 1);

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    btns->button(QDialogButtonBox::Ok)->setText("完成");
    btns->button(QDialogButtonBox::Cancel)->setText("取消");
    layout->addWidget(btns);
    connect(btns, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) return;
    const QString name = nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::information(this, "创建群聊", "群名称不能为空。");
        return;
    }

    QStringList memberIds;
    for (int i = 0; i < list->count(); ++i) {
        QListWidgetItem* item = list->item(i);
        auto* cb = qobject_cast<QCheckBox*>(list->itemWidget(item));
        if (!cb || !cb->isChecked()) continue;
        const QString userId = cb->property("userId").toString().trimmed();
        if (!userId.isEmpty() && userId != m_currentUserId) {
            memberIds.append(userId);
        }
    }
    if (memberIds.isEmpty()) {
        QMessageBox::information(this, "创建群聊", "请至少选择一位好友。");
        return;
    }

    swift::zone::GroupCreatePayload req;
    req.set_creator_id(m_currentUserId.toStdString());
    req.set_group_name(name.toStdString());
    for (const QString& id : memberIds) {
        req.add_member_ids(id.toStdString());
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
    if (!m_protocol || !m_contactWidget || groupId.isEmpty()) return;
    QList<QPair<QString, QString>> candidates;
    const QList<ContactWidget::FriendItem> friends = m_contactWidget->friends();
    for (const auto& f : friends) {
        const QString id = f.friendId.trimmed();
        if (id.isEmpty() || id == m_currentUserId) continue;
        const QString display = f.remark.trimmed().isEmpty()
            ? (f.nickname.trimmed().isEmpty() ? id : f.nickname.trimmed())
            : f.remark.trimmed();
        candidates.append({id, display});
    }
    const QStringList pickedIds = pickUsersForAction("邀请群成员", "选择要邀请的好友", candidates);
    if (pickedIds.isEmpty()) return;

    swift::zone::GroupInviteMembersPayload req;
    req.set_group_id(groupId.toStdString());
    req.set_inviter_id(m_currentUserId.toStdString());
    for (const QString& id : pickedIds) {
        req.add_member_ids(id.toStdString());
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
    if (!m_protocol || !m_contactWidget || groupId.isEmpty()) return;
    QList<QPair<QString, QString>> candidates;
    const QList<ContactWidget::GroupMemberItem> members = m_contactWidget->groupMembers();
    for (const auto& m : members) {
        const QString id = m.userId.trimmed();
        if (id.isEmpty() || id == m_currentUserId) continue;
        const QString display = m.nickname.trimmed().isEmpty() ? id : m.nickname.trimmed();
        candidates.append({id, display});
    }
    const QString memberId = pickUserForAction("移出群成员", "选择要移出的成员", candidates);
    if (memberId.isEmpty()) return;

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
        saveConversationsToLocal();
        loadUserGroups();
    });
}

void MainWindow::dismissGroup(const QString& groupId) {
    if (!m_protocol || groupId.isEmpty()) return;
    const auto reply = QMessageBox::question(this, "删除群聊", QString("确定解散并删除群聊 %1 吗？").arg(groupId));
    if (reply != QMessageBox::Yes) return;

    swift::zone::GroupDismissPayload req;
    req.set_group_id(groupId.toStdString());
    req.set_operator_id(m_currentUserId.toStdString());
    std::string payload;
    if (!req.SerializeToString(&payload)) return;
    m_protocol->sendRequest("group.dismiss",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this, groupId](int code, const QByteArray&, const QString& message) {
        if (code != 0) {
            const QString err = message.trimmed().isEmpty()
                ? QString("删除群聊失败，错误码: %1").arg(code)
                : message;
            QMessageBox::warning(this, "删除群聊失败", err);
            return;
        }
        const QString key = convKey(groupId, 2);
        m_conversationMap.remove(key);
        m_messageMap.remove(key);
        if (m_currentChatType == 2 && m_currentChatId == groupId) {
            m_currentChatId.clear();
            m_chatWidget->setConversation(QString(), 2);
        }
        saveConversationsToLocal();
        loadUserGroups();
    });
}

void MainWindow::deleteConversation(const QString& chatId, int chatType) {
    if (!m_protocol || chatId.isEmpty()) return;
    const auto reply = QMessageBox::question(this, "删除会话", "确定删除当前会话吗？");
    if (reply != QMessageBox::Yes) return;

    swift::zone::ChatDeleteConversationPayload req;
    req.set_chat_id(chatId.toStdString());
    req.set_chat_type(chatType);
    std::string payload;
    if (!req.SerializeToString(&payload)) return;
    m_protocol->sendRequest("chat.delete_conversation",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this, chatId, chatType](int code, const QByteArray&, const QString& message) {
        if (code != 0) {
            const QString err = message.trimmed().isEmpty()
                ? QString("删除会话失败，错误码: %1").arg(code)
                : message;
            QMessageBox::warning(this, "删除会话失败", err);
            return;
        }
        const QString key = convKey(chatId, chatType);
        m_conversationMap.remove(key);
        m_messageMap.remove(key);
        if (m_currentChatId == chatId && m_currentChatType == chatType) {
            m_currentChatId.clear();
            m_chatWidget->setConversation(QString(), chatType);
        }
        syncConversations();
    });
}

void MainWindow::showAddFriendDialog() {
    if (!m_protocol) return;

    QDialog dialog(this);
    dialog.setWindowTitle("添加好友");
    dialog.resize(520, 460);
    QPointer<QDialog> dialogGuard(&dialog);
    auto* root = new QVBoxLayout(&dialog);
    root->setContentsMargins(16, 14, 16, 12);
    root->setSpacing(10);

    auto* searchRow = new QHBoxLayout();
    auto* keywordEdit = new QLineEdit(&dialog);
    keywordEdit->setPlaceholderText("输入好友名称或用户ID");
    keywordEdit->setClearButtonEnabled(true);
    keywordEdit->setMinimumHeight(34);
    auto* searchBtn = new QPushButton("搜索", &dialog);
    searchBtn->setCursor(Qt::PointingHandCursor);
    searchBtn->setMinimumSize(78, 34);
    searchRow->addWidget(keywordEdit, 1);
    searchRow->addWidget(searchBtn);
    root->addLayout(searchRow);

    auto* hintLabel = new QLabel("输入关键词后点击搜索", &dialog);
    hintLabel->setStyleSheet("color:#7f8694;font-size:12px;");
    root->addWidget(hintLabel);

    auto* resultList = new QListWidget(&dialog);
    resultList->setFrameShape(QFrame::NoFrame);
    resultList->setStyleSheet(
        "QListWidget{background:#fff;border:1px solid #e1e6ef;border-radius:10px;}"
        "QListWidget::item{height:56px;}"
    );
    root->addWidget(resultList, 1);

    auto requestSeq = std::make_shared<int>(0);
    auto renderResults = [this, keywordEdit, resultList, hintLabel, searchBtn, dialogGuard, requestSeq]() {
        resultList->clear();
        const QString key = keywordEdit->text().trimmed();
        if (key.isEmpty()) {
            hintLabel->setText("输入关键词后点击搜索");
            return;
        }

        const int seq = ++(*requestSeq);
        searchBtn->setEnabled(false);
        hintLabel->setText("搜索中...");

        swift::zone::FriendSearchPayload req;
        req.set_keyword(key.toStdString());
        req.set_limit(30);
        std::string payload;
        if (!req.SerializeToString(&payload)) {
            searchBtn->setEnabled(true);
            hintLabel->setText("请求构造失败，请重试");
            return;
        }
        m_protocol->sendRequest("friend.search",
                                QByteArray(payload.data(), static_cast<int>(payload.size())),
                                [this, dialogGuard, resultList, hintLabel, searchBtn, requestSeq, seq](int code, const QByteArray& data, const QString& message) {
            if (!dialogGuard) return;
            if (seq != *requestSeq) return;
            searchBtn->setEnabled(true);
            resultList->clear();
            if (code != 0) {
                const QString err = message.trimmed().isEmpty()
                    ? QString("搜索失败，错误码: %1").arg(code)
                    : message;
                hintLabel->setText(err);
                return;
            }
            swift::zone::FriendSearchResponsePayload resp;
            if (!resp.ParseFromArray(data.data(), data.size())) {
                hintLabel->setText("搜索结果解析失败");
                return;
            }
            int shown = 0;
            for (const auto& u : resp.users()) {
                const QString uid = QString::fromStdString(u.user_id()).trimmed();
                if (uid.isEmpty()) continue;
                if (m_contactWidget && !m_contactWidget->friendById(uid).friendId.isEmpty()) continue;
                const QString username = QString::fromStdString(u.username()).trimmed();
                const QString nickname = QString::fromStdString(u.nickname()).trimmed();
                const QString signature = QString::fromStdString(u.signature()).trimmed();
                const QString showName = nickname.isEmpty() ? (username.isEmpty() ? uid : username) : nickname;

                auto* item = new QListWidgetItem(resultList);
                auto* row = new QWidget(resultList);
                auto* rowLayout = new QHBoxLayout(row);
                rowLayout->setContentsMargins(10, 4, 10, 4);
                rowLayout->setSpacing(8);
                auto* name = new QLabel(QString("%1 (%2)").arg(showName, uid), row);
                name->setStyleSheet("color:#2a2f3a;font-size:13px;");
                auto* source = new QLabel(signature.isEmpty() ? "可添加" : signature, row);
                source->setStyleSheet("color:#7f8694;font-size:11px;");
                source->setWordWrap(true);
                auto* textWrap = new QVBoxLayout();
                textWrap->setSpacing(2);
                textWrap->setContentsMargins(0, 0, 0, 0);
                textWrap->addWidget(name);
                textWrap->addWidget(source);
                auto* addBtn = new QPushButton("添加", row);
                addBtn->setCursor(Qt::PointingHandCursor);
                addBtn->setFixedHeight(28);
                addBtn->setStyleSheet(
                    "QPushButton{background:#07c160;color:#fff;border:none;border-radius:6px;padding:0 12px;font-size:12px;font-weight:600;}"
                    "QPushButton:hover{background:#06ad56;}"
                    "QPushButton:disabled{background:#97d9b5;color:#e7f8ee;}"
                );
                rowLayout->addLayout(textWrap, 1);
                rowLayout->addWidget(addBtn);
                resultList->setItemWidget(item, row);
                QObject::connect(addBtn, &QPushButton::clicked, dialogGuard.data(), [this, uid, addBtn]() {
                    addBtn->setEnabled(false);
                    swift::zone::FriendAddPayload req;
                    req.set_user_id(m_currentUserId.toStdString());
                    req.set_friend_id(uid.toStdString());
                    req.set_remark(QString("来自 %1 的好友申请").arg(m_currentUserId).toStdString());
                    std::string payload;
                    if (!req.SerializeToString(&payload)) {
                        addBtn->setEnabled(true);
                        return;
                    }
                    m_protocol->sendRequest("friend.add",
                                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                                            [this, uid, addBtn](int code, const QByteArray&, const QString& message) {
                        if (code != 0) {
                            addBtn->setEnabled(true);
                            const QString err = message.trimmed().isEmpty()
                                ? QString("发送好友申请失败，错误码: %1").arg(code)
                                : message;
                            QMessageBox::warning(this, "添加好友失败", err);
                            return;
                        }
                        addBtn->setText("已发送");
                        QMessageBox::information(this, "添加好友", QString("已向 %1 发送好友申请。").arg(uid));
                        loadFriendRequests();
                    });
                });
                ++shown;
            }
            hintLabel->setText(shown > 0 ? QString("找到 %1 个用户").arg(shown) : QString("没有匹配结果"));
        });
    };

    connect(searchBtn, &QPushButton::clicked, &dialog, [renderResults]() {
        renderResults();
    });
    connect(keywordEdit, &QLineEdit::returnPressed, &dialog, [renderResults]() {
        renderResults();
    });

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    buttons->button(QDialogButtonBox::Close)->setText("关闭");
    root->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    dialog.exec();
}

void MainWindow::showBlacklistDialog() {
    BlacklistDialog* dialog = new BlacklistDialog(this);
    dialog->setBlockedUsers(m_blacklistMap);
    
    connect(dialog, &BlacklistDialog::addUserRequested, this, [this](const QString& userId, const QString& userName) {
        blockUser(userId);
    });
    
    connect(dialog, &BlacklistDialog::removeUserRequested, this, [this](const QString& userId) {
        unblockUser(userId);
    });
    
    connect(dialog, &QDialog::finished, this, [this, dialog]() {
        dialog->deleteLater();
    });
    
    dialog->exec();
}

void MainWindow::loadBlacklist() {
    if (!m_protocol) return;
    
    // 暂时简化处理，直接从好友列表中过滤被拉黑的用户
    // 实际应该调用 friend.get_block_list 接口
    m_blacklistMap.clear();
}

void MainWindow::blockUser(const QString& userId) {
    if (!m_protocol || userId.isEmpty()) return;
    
    swift::zone::FriendBlockPayload req;
    req.set_user_id(m_currentUserId.toStdString());
    req.set_target_id(userId.toStdString());
    std::string payload;
    if (!req.SerializeToString(&payload)) return;
    
    m_protocol->sendRequest("friend.block",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this, userId](int code, const QByteArray&, const QString& message) {
        if (code != 0) {
            const QString err = message.trimmed().isEmpty()
                ? QString("拉黑用户失败，错误码：%1").arg(code)
                : message;
            QMessageBox::warning(this, "操作失败", err);
            return;
        }
        
        // 更新本地黑名单
        m_blacklistMap[userId] = userId;
        QMessageBox::information(this, "操作成功", QString("已将 %1 加入黑名单。").arg(userId));
    });
}

void MainWindow::unblockUser(const QString& userId) {
    if (!m_protocol || userId.isEmpty()) return;
    
    // 使用 FriendBlockPayload 的相反操作
    swift::zone::FriendBlockPayload req;
    req.set_user_id(m_currentUserId.toStdString());
    req.set_target_id(userId.toStdString());
    std::string payload;
    if (!req.SerializeToString(&payload)) return;
    
    m_protocol->sendRequest("friend.unblock",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this, userId](int code, const QByteArray&, const QString& message) {
        if (code != 0) {
            const QString err = message.trimmed().isEmpty()
                ? QString("移除黑名单失败，错误码：%1").arg(code)
                : message;
            QMessageBox::warning(this, "操作失败", err);
            return;
        }
        
        // 从本地黑名单中移除
        m_blacklistMap.remove(userId);
        QMessageBox::information(this, "操作成功", QString("已将 %1 从黑名单移除。").arg(userId));
    });
}

QString MainWindow::pickUserForAction(const QString& title,
                                      const QString& hint,
                                      const QList<QPair<QString, QString>>& candidates) const {
    if (candidates.isEmpty()) {
        QMessageBox::information(const_cast<MainWindow*>(this), title, "没有可选成员。");
        return QString();
    }
    QDialog dialog(const_cast<MainWindow*>(this));
    dialog.setWindowTitle(title);
    dialog.resize(420, 440);
    auto* root = new QVBoxLayout(&dialog);
    root->setContentsMargins(16, 16, 16, 12);
    root->setSpacing(8);
    root->addWidget(new QLabel(hint, &dialog));
    auto* list = new QListWidget(&dialog);
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    for (const auto& c : candidates) {
        auto* item = new QListWidgetItem(c.second, list);
        item->setData(Qt::UserRole, c.first);
    }
    root->addWidget(list, 1);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText("确定");
    buttons->button(QDialogButtonBox::Cancel)->setText("取消");
    root->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted || !list->currentItem()) return QString();
    return list->currentItem()->data(Qt::UserRole).toString();
}

QStringList MainWindow::pickUsersForAction(const QString& title,
                                           const QString& hint,
                                           const QList<QPair<QString, QString>>& candidates) const {
    QStringList result;
    if (candidates.isEmpty()) {
        QMessageBox::information(const_cast<MainWindow*>(this), title, "没有可选成员。");
        return result;
    }
    QDialog dialog(const_cast<MainWindow*>(this));
    dialog.setWindowTitle(title);
    dialog.resize(420, 500);
    auto* root = new QVBoxLayout(&dialog);
    root->setContentsMargins(16, 16, 16, 12);
    root->setSpacing(8);
    root->addWidget(new QLabel(hint, &dialog));
    auto* list = new QListWidget(&dialog);
    list->setSelectionMode(QAbstractItemView::NoSelection);
    for (const auto& c : candidates) {
        auto* item = new QListWidgetItem(list);
        auto* box = new QCheckBox(c.second, list);
        box->setProperty("userId", c.first);
        list->setItemWidget(item, box);
    }
    root->addWidget(list, 1);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText("确定");
    buttons->button(QDialogButtonBox::Cancel)->setText("取消");
    root->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return result;
    for (int i = 0; i < list->count(); ++i) {
        QListWidgetItem* item = list->item(i);
        auto* box = qobject_cast<QCheckBox*>(list->itemWidget(item));
        if (box && box->isChecked()) {
            const QString id = box->property("userId").toString().trimmed();
            if (!id.isEmpty()) result.append(id);
        }
    }
    return result;
}

void MainWindow::loadHistory(const QString& chatId, int chatType) {
    if (!m_appService) return;
    m_appService->sendChatGetHistory(chatId, chatType, QString(), 50,
                                     [this, chatId, chatType](int code, const QString&, const swift::zone::ChatGetHistoryResponsePayload& resp) {
        if (code != 0) return;

        QList<Message> messages;
        QSet<QString> serverMsgIds;
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
            if (!msg.msgId.isEmpty()) serverMsgIds.insert(msg.msgId);
        }
        const QString key = convKey(chatId, chatType);
        for (const auto& local : m_messageMap.value(key)) {
            if (!local.msgId.isEmpty() && !serverMsgIds.contains(local.msgId))
                messages.append(local);
        }
        std::sort(messages.begin(), messages.end(),
                  [](const Message& a, const Message& b) { return a.timestamp < b.timestamp; });
        m_messageMap[key] = messages;
        ChatStorage::instance().mergeAndSaveMessages(m_currentUserId, chatId, chatType, messages, 500);
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
                const QString peerId = (msg.chatType == 1)
                    ? (msg.isSelf ? msg.toId : msg.fromUserId)
                    : msg.toId;
                if (convKey(peerId, msg.chatType) == currentKey) {
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
    const QString peerId = (msg.chatType == 1)
        ? (msg.isSelf ? msg.toId : msg.fromUserId)
        : msg.toId;
    const QString key = convKey(peerId, msg.chatType);
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
    conv.chatId = peerId;
    conv.chatType = msg.chatType;
    if (conv.peerName.isEmpty()) conv.peerName = peerId;
    if (msg.timestamp >= conv.updatedAt) {
        conv.lastMessage = msg;
        conv.updatedAt = msg.timestamp;
    }
    if (!(peerId == m_currentChatId && msg.chatType == m_currentChatType) && !msg.isSelf) {
        conv.unreadCount += 1;
    }
    m_conversationMap[key] = conv;
    m_contactWidget->upsertConversation(conv);
    saveMessagesToLocal(peerId, msg.chatType);
    saveConversationsToLocal();
    return true;
}

void MainWindow::sendChatMessage(const QString& content) {
    if (!m_protocol || m_currentChatId.isEmpty() || content.isEmpty()) return;
    if (!ensureGatewayConnected(QStringLiteral("发送消息"))) return;
    if (!ensureSessionReady(QStringLiteral("发送消息"))) return;

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
    LogInfo("[MainWindow] send text message req, to=" << chatId.toStdString()
            << ", chat_type=" << chatType
            << ", client_msg_id=" << localMsgId.toStdString()
            << ", content_len=" << content.size());
    m_protocol->sendRequest("chat.send_message",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this, key, localMsgId, chatId, chatType](int code, const QByteArray& data, const QString& message) {
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
            if (code == 307 || message.contains("token invalid", Qt::CaseInsensitive)) {
                m_sessionReady = false;
                LogWarning("[MainWindow] text message hit session invalid, trigger re-validation");
                triggerSessionValidation();
            }
            LogWarning("[MainWindow] send text message failed, code=" << code
                       << ", client_msg_id=" << localMsgId.toStdString()
                       << ", message=" << message.toStdString());
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
            LogWarning("[MainWindow] send text message response invalid, client_msg_id="
                       << localMsgId.toStdString()
                       << ", parse_ok=" << resp.ParseFromArray(data.data(), data.size())
                       << ", success=" << resp.success());
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
        LogInfo("[MainWindow] send text message done, client_msg_id="
                << localMsgId.toStdString()
                << ", server_msg_id=" << messages[idx].msgId.toStdString()
                << ", ts=" << messages[idx].timestamp);

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
        saveMessagesToLocal(chatId, chatType);
        saveConversationsToLocal();
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
    if (!ensureGatewayConnected(QStringLiteral("发送文件"))) return;
    if (!ensureSessionReady(QStringLiteral("发送文件"))) return;
    const QString targetChatId = m_currentChatId;
    const int targetChatType = m_currentChatType;
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        LogWarning("[MainWindow] send file aborted: invalid file path=" << filePath.toStdString());
        QMessageBox::warning(this, "文件发送失败", "选择的文件不存在或不可用。");
        return;
    }
    LogInfo("[MainWindow] send file begin, path=" << filePath.toStdString()
            << ", to=" << targetChatId.toStdString()
            << ", chat_type=" << targetChatType
            << ", size=" << fileInfo.size());

    swift::zone::FileGetUploadTokenPayload tokenReq;
    tokenReq.set_user_id(m_currentUserId.toStdString());
    tokenReq.set_file_name(fileInfo.fileName().toStdString());
    tokenReq.set_file_size(fileInfo.size());

    auto requestUploadToken = std::make_shared<std::function<void(bool)>>();
    *requestUploadToken = [this, filePath, fileInfo, targetChatId, targetChatType, requestUploadToken](bool retried) {
        LogInfo("[MainWindow] request upload token, retried=" << retried
                << ", file=" << filePath.toStdString());
        swift::zone::FileGetUploadTokenPayload tokenReqInner;
        tokenReqInner.set_user_id(m_currentUserId.toStdString());
        tokenReqInner.set_file_name(fileInfo.fileName().toStdString());
        tokenReqInner.set_file_size(fileInfo.size());

        std::string tokenPayload;
        if (!tokenReqInner.SerializeToString(&tokenPayload)) return;
        m_protocol->sendRequest("file.get_upload_token",
                                QByteArray(tokenPayload.data(), static_cast<int>(tokenPayload.size())),
                                [this, filePath, fileInfo, targetChatId, targetChatType, retried, requestUploadToken]
                                (int code, const QByteArray& data, const QString& message) {
            if (code == -1 || code == -2) {
                if (!retried) {
                    LogWarning("[MainWindow] upload token request network fail, will retry once, code=" << code);
                    ensureGatewayConnected(QStringLiteral("发送文件"));
                    QTimer::singleShot(600, this, [requestUploadToken]() {
                        (*requestUploadToken)(true);
                    });
                    return;
                }
                LogWarning("[MainWindow] upload token request failed after retry, code=" << code);
                const QString netErr = (code == -1)
                    ? QStringLiteral("与网关连接已断开，请等待自动重连后重试。")
                    : QStringLiteral("获取上传令牌请求超时，请稍后重试。");
                QMessageBox::warning(this, "文件发送失败", netErr);
                return;
            }
            if (code != 0) {
                LogWarning("[MainWindow] upload token request failed, code=" << code
                           << ", message=" << message.toStdString());
                const QString err = message.trimmed().isEmpty()
                    ? QString("获取上传令牌失败，错误码: %1").arg(code)
                    : message;
                QMessageBox::warning(this, "文件发送失败", err);
                return;
            }

            swift::zone::FileGetUploadTokenResponsePayload tokenResp;
            if (!tokenResp.ParseFromArray(data.data(), data.size()) || !tokenResp.success()) {
                LogWarning("[MainWindow] upload token response invalid, success=" << tokenResp.success()
                           << ", error=" << tokenResp.error());
                const QString err = QString::fromStdString(tokenResp.error()).trimmed();
                QMessageBox::warning(this, "文件发送失败", err.isEmpty() ? "上传令牌响应无效。" : err);
                return;
            }

            const QString uploadUrl = QString::fromStdString(tokenResp.upload_url());
            const QString uploadToken = QString::fromStdString(tokenResp.upload_token());
            QString mediaUrl;
            QString uploadError;
            if (!uploadFileByHttp(uploadUrl, uploadToken, filePath, &mediaUrl, &uploadError)) {
                LogWarning("[MainWindow] file upload http failed, file=" << filePath.toStdString()
                           << ", error=" << uploadError.toStdString());
                QMessageBox::warning(this, "文件发送失败", uploadError.isEmpty() ? "文件上传失败。" : uploadError);
                return;
            }
            LogInfo("[MainWindow] file upload http done, file=" << filePath.toStdString()
                    << ", media_url=" << mediaUrl.toStdString());

            swift::zone::ChatSendMessagePayload req;
            req.set_from_user_id(m_currentUserId.toStdString());
            req.set_to_id(targetChatId.toStdString());
            req.set_chat_type(targetChatType);
            req.set_content(fileInfo.fileName().toStdString());
            req.set_media_type("file");
            req.set_media_url(mediaUrl.toStdString());
            req.set_file_size(fileInfo.size());
            req.set_client_msg_id(QString("c_file_%1").arg(QDateTime::currentMSecsSinceEpoch()).toStdString());

            std::string payload;
            if (!req.SerializeToString(&payload)) return;
            LogInfo("[MainWindow] send file message req, to=" << targetChatId.toStdString()
                    << ", chat_type=" << targetChatType
                    << ", file_name=" << fileInfo.fileName().toStdString());
            m_protocol->sendRequest("chat.send_message",
                                    QByteArray(payload.data(), static_cast<int>(payload.size())),
                                    [this, mediaUrl, fileInfo, targetChatId, targetChatType]
                                    (int sendCode, const QByteArray& sendData, const QString& sendMessage) {
                if (sendCode == -1 || sendCode == -2) {
                    LogWarning("[MainWindow] send file message network fail, code=" << sendCode
                               << ", file=" << fileInfo.fileName().toStdString());
                    const QString netErr = (sendCode == -1)
                        ? QStringLiteral("文件已上传，但发送消息时连接断开，请重连后重试发送。")
                        : QStringLiteral("发送文件消息超时，请稍后重试。");
                    QMessageBox::warning(this, "文件发送失败", netErr);
                    return;
                }
                if (sendCode != 0) {
                    if (sendCode == 307 || sendMessage.contains("token invalid", Qt::CaseInsensitive)) {
                        m_sessionReady = false;
                        LogWarning("[MainWindow] file message hit session invalid, trigger re-validation");
                        triggerSessionValidation();
                    }
                    LogWarning("[MainWindow] send file message failed, code=" << sendCode
                               << ", message=" << sendMessage.toStdString());
                    const QString err = sendMessage.trimmed().isEmpty()
                        ? QString("发送文件消息失败，错误码: %1").arg(sendCode)
                        : sendMessage;
                    QMessageBox::warning(this, "文件发送失败", err);
                    return;
                }
                swift::zone::ChatSendMessageResponsePayload resp;
                if (!resp.ParseFromArray(sendData.data(), sendData.size()) || !resp.success()) {
                    LogWarning("[MainWindow] send file message response invalid, success=" << resp.success()
                               << ", error=" << resp.error());
                    QMessageBox::warning(this, "文件发送失败", "服务端未返回成功结果。");
                    return;
                }

                Message msg;
                msg.msgId = QString::fromStdString(resp.msg_id());
                msg.fromUserId = m_currentUserId;
                msg.toId = targetChatId;
                msg.chatType = targetChatType;
                msg.content = fileInfo.fileName();
                msg.mediaType = "file";
                msg.mediaUrl = mediaUrl;
                msg.timestamp = resp.timestamp();
                msg.isSelf = true;
                LogInfo("[MainWindow] send file message done, server_msg_id=" << msg.msgId.toStdString()
                        << ", file_name=" << msg.content.toStdString()
                        << ", ts=" << msg.timestamp);

                const QString key = convKey(targetChatId, targetChatType);
                m_messageMap[key].append(msg);
                if (m_chatWidget->chatId() == targetChatId && m_chatWidget->chatType() == targetChatType) {
                    m_chatWidget->appendMessage(msg);
                }

                auto convIt = m_conversationMap.find(key);
                if (convIt != m_conversationMap.end()) {
                    convIt->lastMessage = msg;
                    convIt->updatedAt = msg.timestamp;
                    m_contactWidget->upsertConversation(*convIt);
                }
                saveMessagesToLocal(targetChatId, targetChatType);
                saveConversationsToLocal();
            });
        });
    };
    (*requestUploadToken)(false);
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
    QList<Message> local = ChatStorage::instance().loadMessages(m_currentUserId, chatId, chatType);
    if (!local.isEmpty()) {
        for (auto& m : local) m.isSelf = (m.fromUserId == m_currentUserId);
        m_messageMap[key] = local;
        m_chatWidget->setMessages(local);
        sendMarkRead();
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

void MainWindow::onConversationMoreRequested(const QPoint& globalPos) {
    if (m_currentChatId.isEmpty()) return;
    QMenu menu(this);
    if (m_currentChatType == 1) {
        QAction* deleteFriendAction = menu.addAction("删除好友");
        QAction* deleteConvAction = menu.addAction("删除会话");
        QAction* picked = menu.exec(globalPos);
        if (!picked) return;
        if (picked == deleteFriendAction) {
            m_profileUserId = m_currentChatId;
            removeCurrentFriend();
            return;
        }
        if (picked == deleteConvAction) {
            deleteConversation(m_currentChatId, m_currentChatType);
            return;
        }
        return;
    }

    QAction* inviteAction = menu.addAction("邀请成员");
    QAction* kickAction = menu.addAction("移出成员");
    QAction* leaveAction = menu.addAction("退出群聊");
    QAction* dismissAction = menu.addAction("删除群聊");
    menu.addSeparator();
    QAction* deleteConvAction = menu.addAction("删除会话");
    QAction* picked = menu.exec(globalPos);
    if (!picked) return;
    if (picked == inviteAction) {
        inviteGroupMembers(m_currentChatId);
    } else if (picked == kickAction) {
        removeGroupMember(m_currentChatId);
    } else if (picked == leaveAction) {
        leaveGroup(m_currentChatId);
    } else if (dismissAction && picked == dismissAction) {
        dismissGroup(m_currentChatId);
    } else if (picked == deleteConvAction) {
        deleteConversation(m_currentChatId, m_currentChatType);
    }
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
        // 私聊: chat_id 为对方 id；群聊: chat_id 为 group_id。legacy 格式 chat_id 即会话 peer
        const QString peerId = m.toId;  // legacy 中 chat_id 已是 peer
        const QString key = convKey(peerId, m.chatType);
        m_messageMap[key].append(m);
        if (peerId == m_currentChatId && m.chatType == m_currentChatType) {
            m_chatWidget->appendMessage(m);
            sendMarkRead();
        } else if (!m.isSelf) {
            m_contactWidget->increaseUnreadForChat(peerId);
        }
        saveMessagesToLocal(peerId, m.chatType);
        saveConversationsToLocal();
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
    // 私聊: 接收方时 peer=发送方(from_user_id)，发送方同步时 peer=接收方(to_id)；群聊: peer=to_id
    const QString peerId = (msg.chatType == 1)
        ? (msg.isSelf ? msg.toId : msg.fromUserId)
        : msg.toId;
    const QString key = convKey(peerId, msg.chatType);
    m_messageMap[key].append(msg);

    Conversation conv = m_conversationMap.value(key);
    conv.chatId = peerId;
    conv.chatType = msg.chatType;
    if (conv.peerName.isEmpty()) conv.peerName = peerId;
    conv.lastMessage = msg;
    conv.updatedAt = msg.timestamp;
    if (!(peerId == m_currentChatId && msg.chatType == m_currentChatType) && !msg.isSelf) {
        conv.unreadCount += 1;
    }
    m_conversationMap[key] = conv;
    m_contactWidget->upsertConversation(conv);
    saveMessagesToLocal(peerId, msg.chatType);
    saveConversationsToLocal();

    if (peerId == m_currentChatId && msg.chatType == m_currentChatType) {
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
    if (chatId == m_currentChatId && m_chatWidget) {
        // 简单的已读显示：显示谁读了最后一条消息
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
        LogInfo("[MainWindow] friend.request push, from_user=" << fromUserId.toStdString()
                << ", from_nickname=" << fromNickname.toStdString());
    }
    QMessageBox::information(this, "好友申请", prompt);
    loadFriendRequests();
    loadFriends();
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
        LogInfo("[MainWindow] friend.accepted push, friend_id=" << userId.toStdString()
                << ", nickname=" << nickname.toStdString());
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

bool MainWindow::ensureGatewayConnected(const QString& actionText) {
    if (!m_wsClient) return true;
    if (m_wsClient->isConnected()) return true;
    const QString serverUrl = Settings::instance().serverUrl().trimmed();
    LogWarning("[MainWindow] gateway disconnected before action=" << actionText.toStdString()
               << ", reconnect_url=" << serverUrl.toStdString());
    if (!serverUrl.isEmpty()) {
        m_wsClient->connect(serverUrl);
    }
    QMessageBox::warning(
        this,
        "网络未连接",
        QString("当前与网关连接已断开，正在自动重连，请稍后再%1。").arg(actionText)
    );
    return false;
}

bool MainWindow::ensureSessionReady(const QString& actionText) {
    if (m_sessionReady) return true;
    triggerSessionValidation();
    QMessageBox::information(
        this,
        "会话恢复中",
        QString("连接已恢复，正在校验登录状态，请稍后再%1。").arg(actionText)
    );
    return false;
}

void MainWindow::triggerSessionValidation() {
    if (!m_protocol || m_sessionValidationInFlight) return;
    if (m_wsClient && !m_wsClient->isConnected()) {
        LogWarning("[MainWindow] skip auth.validate_token: websocket not connected");
        return;
    }
    const QString token = Settings::instance().savedToken().trimmed();
    if (token.isEmpty()) {
        m_sessionReady = false;
        LogWarning("[MainWindow] no saved token, cannot validate session");
        return;
    }

    swift::zone::AuthValidateTokenPayload req;
    req.set_token(token.toStdString());
    std::string payload;
    if (!req.SerializeToString(&payload)) {
        m_sessionReady = false;
        LogWarning("[MainWindow] auth.validate_token serialize failed");
        return;
    }

    m_sessionValidationInFlight = true;
    LogInfo("[MainWindow] send auth.validate_token for session recovery");
    m_protocol->sendRequest("auth.validate_token",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this](int code, const QByteArray& data, const QString& message) {
        m_sessionValidationInFlight = false;
        if (code != 0) {
            m_sessionReady = false;
            LogWarning("[MainWindow] auth.validate_token failed, code=" << code
                       << ", message=" << message.toStdString());
            return;
        }
        swift::zone::AuthValidateTokenResponsePayload resp;
        if (!resp.ParseFromArray(data.data(), data.size())) {
            m_sessionReady = false;
            LogWarning("[MainWindow] auth.validate_token response parse failed");
            return;
        }
        const QString uid = QString::fromStdString(resp.user_id()).trimmed();
        if (uid.isEmpty()) {
            m_sessionReady = false;
            LogWarning("[MainWindow] auth.validate_token returned empty user_id");
            return;
        }
        if (!m_currentUserId.trimmed().isEmpty() && uid != m_currentUserId) {
            m_sessionReady = false;
            LogWarning("[MainWindow] auth.validate_token user mismatch, expected="
                       << m_currentUserId.toStdString() << ", got=" << uid.toStdString());
            return;
        }
        m_sessionReady = true;
        LogInfo("[MainWindow] session validation success, user_id=" << uid.toStdString());
    });
}
