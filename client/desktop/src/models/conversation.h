#pragma once

#include <QString>
#include "message.h"

/**
 * 会话模型
 */
struct Conversation {
    QString chatId;
    int chatType = 1;          // 1=私聊, 2=群聊
    QString peerId;            // 对方 ID（私聊）或群 ID
    QString peerName;
    QString peerAvatar;
    Message lastMessage;
    int unreadCount = 0;
    qint64 updatedAt = 0;
    bool isPinned = false;     // 是否置顶
    bool isMuted = false;      // 是否免打扰
};
