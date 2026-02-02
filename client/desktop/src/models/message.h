#pragma once

#include <QString>
#include <QStringList>

/**
 * 消息模型
 */
struct Message {
    QString msgId;
    QString fromUserId;
    QString toId;
    int chatType = 1;          // 1=私聊, 2=群聊
    QString content;
    QString mediaUrl;
    QString mediaType;         // text, image, file, voice, video
    QStringList mentions;      // @的用户
    QString replyToMsgId;
    qint64 timestamp = 0;
    int status = 0;            // 0=正常, 1=已撤回, 2=已删除
    
    // 本地状态
    bool isSelf = false;
    bool sending = false;
    bool sendFailed = false;
};
