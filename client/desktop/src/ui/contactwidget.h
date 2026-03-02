#pragma once

#include <QWidget>
#include <QList>
#include "models/conversation.h"

class QListWidget;
class QListWidgetItem;

/**
 * 联系人/会话列表组件
 */
class ContactWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit ContactWidget(QWidget *parent = nullptr);
    ~ContactWidget();

    void setConversations(const QList<Conversation>& conversations);
    void upsertConversation(const Conversation& conversation);
    void increaseUnreadForChat(const QString& chatId);
    void setCurrentChat(const QString& chatId);

signals:
    void conversationSelected(const QString& chatId, int chatType);
    void friendSelected(const QString& userId);

private:
    void refreshList();
    void onItemClicked(QListWidgetItem* item);

    QListWidget* m_listWidget = nullptr;
    QList<Conversation> m_conversations;
    QString m_currentChatId;
};
