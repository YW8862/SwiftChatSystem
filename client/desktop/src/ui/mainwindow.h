#pragma once

#include <QMainWindow>
#include <QMap>
#include <QList>
#include <QSet>
#include <QPair>
#include <QPoint>
#include <memory>
#include "models/conversation.h"
#include "models/message.h"

class ProtocolHandler;
class ContactWidget;
class ChatWidget;
class QStackedWidget;
class QLabel;
class QPushButton;
class QToolButton;
class QNetworkAccessManager;
class WebSocketClient;
namespace client { class AppService; }

/**
 * 主窗口
 * 
 * 布局：
 * - 左侧：联系人列表 / 会话列表
 * - 右侧：聊天区域
 * - 顶部：工具栏
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(ProtocolHandler* protocol,
                        WebSocketClient* wsClient,
                        const QString& currentUserId,
                        QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupUi();
    void wireSignals();
    void showProfileDialog();
    void syncConversations();
    void loadLocalConversationsAndMessages();
    void saveConversationsToLocal();
    void saveMessagesToLocal(const QString& chatId, int chatType);
    void loadFriends(const QString& groupId = QString());
    void loadFriendRequests();
    void handleFriendRequest(const QString& requestId, bool accept);
    void loadUserGroups();
    void loadGroupInfo(const QString& groupId);
    void loadGroupMembers(const QString& groupId);
    void createGroup();
    void inviteGroupMembers(const QString& groupId);
    void removeGroupMember(const QString& groupId);
    void leaveGroup(const QString& groupId);
    void dismissGroup(const QString& groupId);
    void deleteConversation(const QString& chatId, int chatType);
    void showAddFriendDialog();
    QString pickUserForAction(const QString& title, const QString& hint, const QList<QPair<QString, QString>>& candidates) const;
    QStringList pickUsersForAction(const QString& title, const QString& hint, const QList<QPair<QString, QString>>& candidates) const;
    void loadHistory(const QString& chatId, int chatType);
    void pullOfflineMessages(int limit = 100);
    bool mergeOfflineMessage(const Message& msg);
    bool uploadFileByHttp(const QString& uploadUrl, const QString& uploadToken, const QString& filePath,
                          QString* mediaUrl, QString* error);
    bool resolveFileDownloadUrl(const Message& msg, QString* fileUrl, QString* fileName, QString* error);
    bool downloadFileToPath(const QString& fileUrl, const QString& savePath, QString* error);
    void openFileMessage(const QString& msgId);
    void sendChatMessage(const QString& content);
    void sendFileMessage(const QString& filePath);
    void retryFailedMessage(const QString& msgId);
    void recallMessage(const QString& msgId);
    void sendMarkRead();
    void refreshFriendProfileCard();
    void removeCurrentFriend();
    bool ensureGatewayConnected(const QString& actionText);
    bool ensureSessionReady(const QString& actionText);
    void triggerSessionValidation();
    QString convKey(const QString& chatId, int chatType) const;

    void onConversationSelected(const QString& chatId, int chatType);
    void onFriendSelected(const QString& userId);
    void onGroupSelected(const QString& groupId);
    void onPushNewMessage(const QByteArray& payload);
    void onPushRecall(const QByteArray& payload);
    void onPushReadReceipt(const QByteArray& payload);
    void onPushFriendRequest(const QByteArray& payload);
    void onPushFriendAccepted(const QByteArray& payload);
    void onConversationMoreRequested(const QPoint& globalPos);

    ProtocolHandler* m_protocol = nullptr;
    WebSocketClient* m_wsClient = nullptr;
    QString m_currentUserId;
    ContactWidget* m_contactWidget = nullptr;
    ChatWidget* m_chatWidget = nullptr;
    QStackedWidget* m_rightStack = nullptr;
    QWidget* m_friendProfilePage = nullptr;
    QLabel* m_profileAvatarLabel = nullptr;
    QLabel* m_profileNameLabel = nullptr;
    QLabel* m_profileIdLabel = nullptr;
    QLabel* m_profileRemarkLabel = nullptr;
    QLabel* m_profileTagStarLabel = nullptr;
    QLabel* m_profileTagGroupLabel = nullptr;
    QLabel* m_profileSourceLabel = nullptr;
    QLabel* m_profileAddedAtLabel = nullptr;
    QToolButton* m_profileMoreBtn = nullptr;
    QPushButton* m_profileSendMsgBtn = nullptr;
    QString m_profileUserId;
    QString m_profileNickname;
    QString m_profileRemark;
    QString m_profileGroupId;
    QString m_profileAvatarUrl;
    qint64 m_profileAddedAt = 0;
    QSet<QString> m_starFriendIds;
    QString m_currentChatId;
    int m_currentChatType = 1;
    QMap<QString, Conversation> m_conversationMap;
    QMap<QString, QList<Message>> m_messageMap;
    QMap<QString, QString> m_readReceiptMap;  // key: chatId, value: userId:lastMsgId
    QString m_offlineCursor;
    bool m_offlinePullInFlight = false;
    bool m_sessionReady = false;
    bool m_sessionValidationInFlight = false;
    QNetworkAccessManager* m_networkManager = nullptr;
    std::unique_ptr<client::AppService> m_appService;
};
