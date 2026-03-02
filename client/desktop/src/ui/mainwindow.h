#pragma once

#include <QMainWindow>
#include <QMap>
#include <QList>
#include "models/conversation.h"
#include "models/message.h"

class ProtocolHandler;
class ContactWidget;
class ChatWidget;

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
    void syncConversations();
    void loadHistory(const QString& chatId, int chatType);
    void sendChatMessage(const QString& content);
    void sendMarkRead();
    QString convKey(const QString& chatId, int chatType) const;

    void onConversationSelected(const QString& chatId, int chatType);
    void onPushNewMessage(const QByteArray& payload);
    void onPushRecall(const QByteArray& payload);
    void onPushReadReceipt(const QByteArray& payload);

    ProtocolHandler* m_protocol = nullptr;
    QString m_currentUserId;
    ContactWidget* m_contactWidget = nullptr;
    ChatWidget* m_chatWidget = nullptr;
    QString m_currentChatId;
    int m_currentChatType = 1;
    QMap<QString, Conversation> m_conversationMap;
    QMap<QString, QList<Message>> m_messageMap;
    QMap<QString, QString> m_readReceiptMap;  // key: chatId, value: userId:lastMsgId
};
