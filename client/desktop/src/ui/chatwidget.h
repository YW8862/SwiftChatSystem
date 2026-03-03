#pragma once

#include <QWidget>
#include <QList>
#include "models/message.h"

class QListWidget;
class QTextEdit;
class QPushButton;
class QLabel;

/**
 * 聊天组件
 * 
 * 包含：
 * - 消息列表
 * - 输入框
 * - 工具栏（表情、文件、截图等）
 */
class ChatWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit ChatWidget(QWidget *parent = nullptr);
    ~ChatWidget();
    
    void setConversation(const QString& chatId, int chatType);
    QString chatId() const { return m_chatId; }
    int chatType() const { return m_chatType; }
    void setCurrentUserId(const QString& userId);
    void setMessages(const QList<Message>& messages);
    void prependHistoryMessages(const QList<Message>& messages);
    void appendMessage(const Message& message);
    void markMessageRecalled(const QString& msgId);
    void setReadReceipt(const QString& userId, const QString& lastReadMsgId);
    void clearReadReceipt();
    
signals:
    void messageSent(const QString& content);
    void fileSelected(const QString& filePath);
    
private slots:
    void onSendClicked();
    void onFileClicked();
    
private:
    QList<Message> normalizeMessages(const QList<Message>& messages) const;
    void refreshMessageList(bool scrollToBottom = true, int preserveScrollValue = -1, int preserveScrollMax = -1);
    void addTimeSeparatorItem(const QString& text);
    void addMessageItem(const Message& message);
    QWidget* buildMessageItemWidget(const Message& message) const;
    void updateHeader();
    QString buildChatTitle() const;

    QString m_chatId;
    int m_chatType = 1;  // 1=私聊, 2=群聊
    QString m_currentUserId;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_subtitleLabel = nullptr;
    QListWidget* m_messageList = nullptr;
    QTextEdit* m_input = nullptr;
    QPushButton* m_sendBtn = nullptr;
    QLabel* m_readReceiptLabel = nullptr;
    QList<Message> m_messages;
};
