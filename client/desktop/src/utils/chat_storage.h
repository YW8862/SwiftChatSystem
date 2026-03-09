#pragma once

#include "models/conversation.h"
#include "models/message.h"
#include <QMap>
#include <QString>

/**
 * 本地聊天数据持久化
 * 会话列表 + 各会话的消息缓存，按用户 ID 存储
 */
class ChatStorage {
public:
    static ChatStorage& instance();

    /// 存储路径（用于调试），基于 QStandardPaths::AppDataLocation
    QString storageDir() const;

    /// 加载当前用户的会话列表
    QList<Conversation> loadConversations(const QString& userId) const;

    /// 加载某会话的消息（最多 maxMessages 条）
    QList<Message> loadMessages(const QString& userId, const QString& chatId, int chatType,
                                int maxMessages = 500) const;

    /// 保存会话列表
    bool saveConversations(const QString& userId, const QList<Conversation>& conversations);

    /// 保存某会话的消息（会覆盖该会话的本地缓存，建议先 load 再 merge 再 save）
    bool saveMessages(const QString& userId, const QString& chatId, int chatType,
                      const QList<Message>& messages);

    /// 合并并保存会话（不覆盖已有，只更新/追加）
    void mergeAndSaveConversations(const QString& userId,
                                   const QMap<QString, Conversation>& conversationMap);

    /// 合并并保存某会话消息（去重、按时间排序后截断）
    void mergeAndSaveMessages(const QString& userId, const QString& chatId, int chatType,
                              const QList<Message>& newMessages, int maxStored = 500);

private:
    ChatStorage() = default;
    ~ChatStorage() = default;
    ChatStorage(const ChatStorage&) = delete;
    ChatStorage& operator=(const ChatStorage&) = delete;

    QString dataFilePath(const QString& userId) const;
};
