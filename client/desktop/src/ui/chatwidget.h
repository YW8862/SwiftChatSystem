#pragma once

#include <QWidget>
#include <QList>
#include <QMap>
#include "models/message.h"

class QListWidget;
class QTextEdit;
class QPushButton;
class QLabel;
class QFrame;
class QSplitter;
class QPoint;
class ImagePreviewDialog;
class ImageViewerDialog;
class MentionPopupDialog;
class ReadReceiptDetailDialog;

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
    void updateReadReceiptDisplay(int chatType, const QMap<QString, QString>& readUsers,
                                 const QMap<QString, QString>& unreadUsers);
    void loadMoreHistoryMessages();
    
signals:
    void messageSent(const QString& content);
    void fileSelected(const QString& filePath);
    void retryMessageRequested(const QString& msgId);
    void recallMessageRequested(const QString& msgId);
    void messageListReachedBottom();
    void fileMessageOpenRequested(const QString& msgId);
    void conversationMoreRequested(const QPoint& globalPos);
    
    // 引用消息相关
    void replyMessageRequested(const QString& targetMsgId, const QString& targetContent,
                              const QString& targetSender, const QString& targetSenderId);
    
private slots:
    void onSendClicked();
    void onFileClicked();
    
private:
    QList<Message> normalizeMessages(const QList<Message>& messages) const;
    void refreshMessageList(bool scrollToBottom = true, int preserveScrollValue = -1, int preserveScrollMax = -1);
    void addTimeSeparatorItem(const QString& text);
    void addMessageItem(const Message& message);
    QWidget* buildMessageItemWidget(const Message& message);
    void updateHeader();
    void updateConversationVisibility();
    QString buildChatTitle() const;
    
    // @提醒相关
    void onInputTextChanged();
    void showMentionPopup();
    void hideMentionPopup();
    void onUserSelected(const QString& userId, const QString& userName);
    
    // 引用消息相关
    void onReplyMessageRequested(const QString& targetMsgId, const QString& targetContent,
                                const QString& targetSender, const QString& targetSenderId);
    void scrollToMessage(const QString& msgId);
    void clearReplyState();
    void showReadReceiptDetail();
    
protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

    QString m_chatId;
    int m_chatType = 1;  // 1=私聊，2=群聊
    QString m_currentUserId;
    ImagePreviewDialog* m_imagePreviewDialog = nullptr;
    ImageViewerDialog* m_imageViewerDialog = nullptr;
    QFrame* m_headerFrame = nullptr;
    QFrame* m_bodyFrame = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_subtitleLabel = nullptr;
    QSplitter* m_verticalSplitter = nullptr;
    QListWidget* m_messageList = nullptr;
    QTextEdit* m_input = nullptr;
    QPushButton* m_sendBtn = nullptr;
    QLabel* m_readReceiptLabel = nullptr;
    QLabel* m_loadMoreLabel = nullptr;  // 加载更多提示
    MentionPopupDialog* m_mentionPopup = nullptr;  // @提醒弹窗
    QList<Message> m_messages;
    
    // 引用消息相关
    QString m_replyToMsgId;         // 当前回复的目标消息 ID
    QString m_replyToContent;       // 当前回复的目标消息内容
    QString m_replyToSender;        // 当前回复的目标消息发送者
    
    // 已读回执相关
    ReadReceiptDetailDialog* m_readReceiptDetailDialog = nullptr;  // 已读详情对话框
    QMap<QString, QString> m_readUsers;     // 已读用户列表
    QMap<QString, QString> m_unreadUsers;   // 未读用户列表
};
