#pragma once

#include <QString>

/**
 * 用户模型
 */
struct User {
    QString userId;
    QString username;
    QString nickname;
    QString avatarUrl;
    QString signature;
    int gender = 0;  // 0=未知, 1=男, 2=女
    bool online = false;
};

/**
 * 好友模型
 */
struct Friend {
    User user;
    QString remark;     // 备注名
    QString groupId;    // 分组 ID
    qint64 addedAt = 0;
};

/**
 * 好友分组
 */
struct FriendGroup {
    QString groupId;
    QString groupName;
    int friendCount = 0;
    int sortOrder = 0;
};
