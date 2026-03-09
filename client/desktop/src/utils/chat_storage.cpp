#include "chat_storage.h"
#include <algorithm>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QSet>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

ChatStorage& ChatStorage::instance() {
    static ChatStorage inst;
    return inst;
}

QString ChatStorage::storageDir() const {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) dir = QDir::homePath() + "/.swiftchat";
    QDir d(dir);
    if (!d.exists()) d.mkpath(".");
    return dir;
}

QString ChatStorage::dataFilePath(const QString& userId) const {
    QString safe = userId;
    for (int i = 0; i < safe.size(); ++i) {
        if (!safe[i].isLetterOrNumber() && safe[i] != '_' && safe[i] != '-')
            safe[i] = '_';
    }
    if (safe.isEmpty()) safe = "default";
    return storageDir() + "/chat_" + safe + ".json";
}

static QJsonObject messageToJson(const Message& m) {
    QJsonObject o;
    o["msgId"] = m.msgId;
    o["fromUserId"] = m.fromUserId;
    o["toId"] = m.toId;
    o["chatType"] = m.chatType;
    o["content"] = m.content;
    o["mediaUrl"] = m.mediaUrl;
    o["mediaType"] = m.mediaType;
    o["timestamp"] = static_cast<double>(m.timestamp);
    o["status"] = m.status;
    o["isSelf"] = m.isSelf;
    o["sending"] = m.sending;
    o["sendFailed"] = m.sendFailed;
    QJsonArray arr;
    for (const auto& x : m.mentions) arr.append(x);
    o["mentions"] = arr;
    o["replyToMsgId"] = m.replyToMsgId;
    return o;
}

static Message jsonToMessage(const QJsonObject& o) {
    Message m;
    m.msgId = o["msgId"].toString();
    m.fromUserId = o["fromUserId"].toString();
    m.toId = o["toId"].toString();
    m.chatType = o["chatType"].toInt(1);
    m.content = o["content"].toString();
    m.mediaUrl = o["mediaUrl"].toString();
    m.mediaType = o["mediaType"].toString();
    m.timestamp = static_cast<qint64>(o["timestamp"].toDouble(0));
    m.status = o["status"].toInt(0);
    m.isSelf = o["isSelf"].toBool(false);
    m.sending = o["sending"].toBool(false);
    m.sendFailed = o["sendFailed"].toBool(false);
    m.replyToMsgId = o["replyToMsgId"].toString();
    for (const auto& v : o["mentions"].toArray())
        m.mentions.append(v.toString());
    return m;
}

static QJsonObject conversationToJson(const Conversation& c) {
    QJsonObject o;
    o["chatId"] = c.chatId;
    o["chatType"] = c.chatType;
    o["peerId"] = c.peerId;
    o["peerName"] = c.peerName;
    o["peerAvatar"] = c.peerAvatar;
    o["unreadCount"] = c.unreadCount;
    o["updatedAt"] = static_cast<double>(c.updatedAt);
    o["isPinned"] = c.isPinned;
    o["isMuted"] = c.isMuted;
    o["lastMsgId"] = c.lastMessage.msgId;
    o["lastContent"] = c.lastMessage.content;
    o["lastTimestamp"] = static_cast<double>(c.lastMessage.timestamp);
    return o;
}

static Conversation jsonToConversation(const QJsonObject& o) {
    Conversation c;
    c.chatId = o["chatId"].toString();
    c.chatType = o["chatType"].toInt(1);
    c.peerId = o["peerId"].toString();
    c.peerName = o["peerName"].toString();
    c.peerAvatar = o["peerAvatar"].toString();
    c.unreadCount = o["unreadCount"].toInt(0);
    c.updatedAt = static_cast<qint64>(o["updatedAt"].toDouble(0));
    c.isPinned = o["isPinned"].toBool(false);
    c.isMuted = o["isMuted"].toBool(false);
    c.lastMessage.msgId = o["lastMsgId"].toString();
    c.lastMessage.content = o["lastContent"].toString();
    c.lastMessage.timestamp = static_cast<qint64>(o["lastTimestamp"].toDouble(0));
    return c;
}

QList<Conversation> ChatStorage::loadConversations(const QString& userId) const {
    QList<Conversation> result;
    QFile f(dataFilePath(userId));
    if (!f.open(QIODevice::ReadOnly)) return result;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject()) return result;
    QJsonObject root = doc.object();
    for (const auto& v : root["conversations"].toArray()) {
        result.append(jsonToConversation(v.toObject()));
    }
    return result;
}

QList<Message> ChatStorage::loadMessages(const QString& userId, const QString& chatId, int chatType,
                                          int maxMessages) const {
    QList<Message> result;
    QFile f(dataFilePath(userId));
    if (!f.open(QIODevice::ReadOnly)) return result;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject()) return result;
    QString key = chatId + "_" + QString::number(chatType);
    for (const auto& v : doc.object()["messages"].toObject()[key].toArray()) {
        result.append(jsonToMessage(v.toObject()));
    }
    if (result.size() > maxMessages) {
        std::sort(result.begin(), result.end(),
                  [](const Message& a, const Message& b) { return a.timestamp < b.timestamp; });
        result = result.mid(result.size() - maxMessages, maxMessages);
    }
    return result;
}

bool ChatStorage::saveConversations(const QString& userId, const QList<Conversation>& conversations) {
    QString path = dataFilePath(userId);
    QJsonObject root;
    QFile f(path);
    if (f.open(QIODevice::ReadOnly)) {
        root = QJsonDocument::fromJson(f.readAll()).object();
        f.close();
    }
    QJsonArray arr;
    for (const auto& c : conversations)
        arr.append(conversationToJson(c));
    root["conversations"] = arr;
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    f.close();
    return true;
}

bool ChatStorage::saveMessages(const QString& userId, const QString& chatId, int chatType,
                               const QList<Message>& messages) {
    QString path = dataFilePath(userId);
    QJsonObject root;
    QFile f(path);
    if (f.open(QIODevice::ReadOnly)) {
        root = QJsonDocument::fromJson(f.readAll()).object();
        f.close();
    }
    QString key = chatId + "_" + QString::number(chatType);
    QJsonArray arr;
    for (const auto& m : messages)
        arr.append(messageToJson(m));
    root["messages"] = root["messages"].toObject();
    QJsonObject msgObj = root["messages"].toObject();
    msgObj[key] = arr;
    root["messages"] = msgObj;
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    f.close();
    return true;
}

void ChatStorage::mergeAndSaveConversations(const QString& userId,
                                            const QMap<QString, Conversation>& conversationMap) {
    QList<Conversation> convs;
    for (auto it = conversationMap.begin(); it != conversationMap.end(); ++it)
        convs.append(it.value());
    std::sort(convs.begin(), convs.end(),
              [](const Conversation& a, const Conversation& b) {
                  return a.updatedAt > b.updatedAt;
              });
    saveConversations(userId, convs);
}

void ChatStorage::mergeAndSaveMessages(const QString& userId, const QString& chatId, int chatType,
                                       const QList<Message>& newMessages, int maxStored) {
    QList<Message> existing = loadMessages(userId, chatId, chatType, maxStored * 2);
    QSet<QString> seen;
    for (const auto& m : existing) {
        if (!m.msgId.isEmpty()) seen.insert(m.msgId);
    }
    for (const auto& m : newMessages) {
        if (!m.msgId.isEmpty() && !seen.contains(m.msgId)) {
            seen.insert(m.msgId);
            existing.append(m);
        }
    }
    std::sort(existing.begin(), existing.end(),
              [](const Message& a, const Message& b) { return a.timestamp < b.timestamp; });
    if (existing.size() > maxStored)
        existing = existing.mid(existing.size() - maxStored, maxStored);
    saveMessages(userId, chatId, chatType, existing);
}
