#pragma once

#include <QWidget>
#include <QList>
#include "models/conversation.h"

class QListWidget;
class QListWidgetItem;
class QLineEdit;
class QPushButton;
class QLabel;
class QStackedWidget;
class QComboBox;

/**
 * 联系人/会话列表组件
 */
class ContactWidget : public QWidget {
    Q_OBJECT
    
public:
    enum class ViewMode {
        Conversations = 0,
        Friends = 1,
        Groups = 2
    };

    struct FriendItem {
        QString friendId;
        QString nickname;
        QString remark;
        QString groupId;
        QString avatarUrl;
        qint64 addedAt = 0;
    };

    struct FriendRequestItem {
        QString requestId;
        QString fromUserId;
        QString fromNickname;
        QString fromAvatarUrl;
        QString remark;
        int status = 0;
        qint64 createdAt = 0;
    };

    struct GroupItem {
        QString groupId;
        QString groupName;
        QString avatarUrl;
        QString ownerId;
        QString announcement;
        int memberCount = 0;
        qint64 createdAt = 0;
    };

    struct GroupMemberItem {
        QString userId;
        QString nickname;
        int role = 0;
        qint64 joinedAt = 0;
    };

    explicit ContactWidget(QWidget *parent = nullptr);
    ~ContactWidget();

    void setViewMode(ViewMode mode);
    ViewMode viewMode() const { return m_viewMode; }

    void setConversations(const QList<Conversation>& conversations);
    void upsertConversation(const Conversation& conversation);
    void increaseUnreadForChat(const QString& chatId);
    void setCurrentChat(const QString& chatId);
    void setFriends(const QList<FriendItem>& friends);
    void setFriendRequests(const QList<FriendRequestItem>& requests);
    void setGroups(const QList<GroupItem>& groups);
    void setCurrentGroupInfo(const GroupItem& groupInfo);
    void setGroupMembers(const QList<GroupMemberItem>& members);
    QString currentSelectedGroupId() const { return m_currentGroupId; }
    FriendItem friendById(const QString& userId) const;
    void updateFriendRemark(const QString& userId, const QString& remark);
    void removeFriendById(const QString& userId);

signals:
    void conversationSelected(const QString& chatId, int chatType);
    void friendSelected(const QString& userId);
    void friendRequestHandled(const QString& requestId, bool accept);
    void groupSelected(const QString& groupId);

private:
    void refreshView();
    void refreshConversationList();
    void refreshFriendList();
    void refreshGroupList();
    void refreshGroupMemberList();
    void applyGroupMemberFilter();
    void applyFilter(const QString& keyword, ViewMode mode);
    QWidget* buildConversationItemWidget(const Conversation& conversation) const;
    QWidget* buildFriendItemWidget(const FriendItem& item) const;
    QWidget* buildFriendRequestItemWidget(const FriendRequestItem& item);
    QWidget* buildGroupItemWidget(const GroupItem& item) const;
    QWidget* buildGroupMemberItemWidget(const GroupMemberItem& item) const;
    void onItemClicked(QListWidgetItem* item);
    void onFriendItemClicked(QListWidgetItem* item);
    void onGroupItemClicked(QListWidgetItem* item);
    void onSearchTextChanged(const QString& text);
    void onGroupMemberSearchChanged(const QString& text);
    void onGroupRoleFilterChanged(int index);
    void updateTabStyles();

    ViewMode m_viewMode = ViewMode::Conversations;
    QPushButton* m_chatTabBtn = nullptr;
    QPushButton* m_friendTabBtn = nullptr;
    QPushButton* m_groupTabBtn = nullptr;
    QLineEdit* m_searchEdit = nullptr;
    QStackedWidget* m_stack = nullptr;
    QListWidget* m_convListWidget = nullptr;
    QListWidget* m_friendRequestListWidget = nullptr;
    QListWidget* m_friendListWidget = nullptr;
    QListWidget* m_groupListWidget = nullptr;
    QListWidget* m_groupMemberListWidget = nullptr;
    QLineEdit* m_groupMemberSearchEdit = nullptr;
    QComboBox* m_groupRoleFilter = nullptr;
    QLabel* m_groupNameLabel = nullptr;
    QLabel* m_groupMetaLabel = nullptr;
    QLabel* m_groupAnnouncementLabel = nullptr;

    QList<Conversation> m_conversations;
    QList<Conversation> m_visibleConversations;
    QList<FriendItem> m_friends;
    QList<FriendItem> m_visibleFriends;
    QList<FriendRequestItem> m_friendRequests;
    QList<GroupItem> m_groups;
    QList<GroupItem> m_visibleGroups;
    QList<GroupMemberItem> m_groupMembers;
    QList<GroupMemberItem> m_visibleGroupMembers;
    GroupItem m_currentGroupInfo;
    QString m_currentChatId;
    QString m_currentGroupId;
};
