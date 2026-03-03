#pragma once

#include <QMainWindow>
#include <QMap>
#include <QList>
#include "models/conversation.h"
#include "models/message.h"

class ProtocolHandler;
class ContactWidget;
class ChatWidget;
class QStackedWidget;
class QLabel;
class QPushButton;

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
                        const QString& currentUserId,
                        QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupUi();
    void wireSignals();
    void showProfileDialog();
    void syncConversations();
    void loadFriends();
    void loadFriendRequests();
    void handleFriendRequest(const QString& requestId, bool accept);
    void loadUserGroups();
    void loadGroupInfo(const QString& groupId);
    void loadGroupMembers(const QString& groupId);
    void loadHistory(const QString& chatId, int chatType);
    void sendChatMessage(const QString& content);
    void sendMarkRead();
    QString convKey(const QString& chatId, int chatType) const;

    void onConversationSelected(const QString& chatId, int chatType);
    void onFriendSelected(const QString& userId);
    void onGroupSelected(const QString& groupId);
    void onPushNewMessage(const QByteArray& payload);
    void onPushRecall(const QByteArray& payload);
    void onPushReadReceipt(const QByteArray& payload);
    void onPushFriendRequest(const QByteArray& payload);
    void onPushFriendAccepted(const QByteArray& payload);

    ProtocolHandler* m_protocol = nullptr;
    QString m_currentUserId;
    ContactWidget* m_contactWidget = nullptr;
    ChatWidget* m_chatWidget = nullptr;
    QStackedWidget* m_rightStack = nullptr;
    QWidget* m_friendProfilePage = nullptr;
    QLabel* m_profileNameLabel = nullptr;
    QLabel* m_profileIdLabel = nullptr;
    QLabel* m_profileRemarkLabel = nullptr;
    QLabel* m_profileSourceLabel = nullptr;
    QLabel* m_profileAddedAtLabel = nullptr;
    QPushButton* m_profileSendMsgBtn = nullptr;
    QString m_profileUserId;
    QString m_currentChatId;
    int m_currentChatType = 1;
    QMap<QString, Conversation> m_conversationMap;
    QMap<QString, QList<Message>> m_messageMap;
    QMap<QString, QString> m_readReceiptMap;  // key: chatId, value: userId:lastMsgId
};
