#include "messageitem.h"

MessageItem::MessageItem(QWidget *parent) : QWidget(parent), m_isSelf(false) {
    // TODO: 初始化 UI
}

MessageItem::~MessageItem() = default;

void MessageItem::setMessage(const QString& msgId, const QString& content,
                              const QString& senderName, bool isSelf, qint64 timestamp) {
    m_msgId = msgId;
    m_isSelf = isSelf;
    // TODO: 设置消息内容
}

void MessageItem::setRecalled(bool recalled) {
    // TODO: 显示已撤回状态
}
