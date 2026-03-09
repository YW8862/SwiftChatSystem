#include "mainwindow.h"

#include "contactwidget.h"
#include "chatwidget.h"
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
#include <QPushButton>
#include <QStackedWidget>
#include <QSplitter>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
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
    wireSignals();
    syncConversations();
    loadFriends();
    loadFriendRequests();
    loadUserGroups();
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
    connect(m_contactWidget, &ContactWidget::friendRequestHandled,
            this, &MainWindow::handleFriendRequest);
    connect(m_chatWidget, &ChatWidget::messageSent,
            this, &MainWindow::sendChatMessage);
    connect(m_chatWidget, &ChatWidget::fileSelected,
            this, &MainWindow::sendFileMessage);

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

void MainWindow::loadFriends() {
    if (!m_protocol || !m_contactWidget) return;
    swift::zone::FriendGetFriendsPayload req;
    req.set_group_id("");
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
                            [this](int code, const QByteArray&) {
        if (code != 0) return;
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

void MainWindow::sendFileMessage(const QString& filePath) {
    if (!m_protocol || m_currentChatId.isEmpty() || filePath.isEmpty()) return;
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        QMessageBox::warning(this, "文件发送失败", "选择的文件不存在或不可用。");
        return;
    }

    swift::zone::ChatSendMessagePayload req;
    req.set_from_user_id(m_currentUserId.toStdString());
    req.set_to_id(m_currentChatId.toStdString());
    req.set_chat_type(m_currentChatType);
    req.set_content(fileInfo.fileName().toStdString());
    req.set_media_type("file");
    req.set_media_url(filePath.toStdString());
    req.set_file_size(fileInfo.size());
    req.set_client_msg_id(QString("c_file_%1").arg(QDateTime::currentMSecsSinceEpoch()).toStdString());

    std::string payload;
    if (!req.SerializeToString(&payload)) return;
    m_protocol->sendRequest("chat.send_message",
                            QByteArray(payload.data(), static_cast<int>(payload.size())),
                            [this, filePath](int code, const QByteArray& data) {
        if (code != 0) {
            QMessageBox::warning(this, "文件发送失败", QString("请求失败，错误码: %1").arg(code));
            return;
        }
        swift::zone::ChatSendMessageResponsePayload resp;
        if (!resp.ParseFromArray(data.data(), data.size()) || !resp.success()) {
            QMessageBox::warning(this, "文件发送失败", "服务端未返回成功结果。");
            return;
        }

        QFileInfo info(filePath);
        Message msg;
        msg.msgId = QString::fromStdString(resp.msg_id());
        msg.fromUserId = m_currentUserId;
        msg.toId = m_currentChatId;
        msg.chatType = m_currentChatType;
        msg.content = info.fileName();
        msg.mediaType = "file";
        msg.mediaUrl = filePath;
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
        return;
    }
    loadHistory(chatId, chatType);
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
    Q_UNUSED(payload);
    loadFriendRequests();
}

void MainWindow::onPushFriendAccepted(const QByteArray& payload) {
    Q_UNUSED(payload);
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
