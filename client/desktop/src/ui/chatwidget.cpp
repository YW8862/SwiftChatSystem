#include "chatwidget.h"

ChatWidget::ChatWidget(QWidget *parent) : QWidget(parent) {
    // TODO: 初始化 UI
}

ChatWidget::~ChatWidget() = default;

void ChatWidget::setConversation(const QString& chatId, int chatType) {
    m_chatId = chatId;
    m_chatType = chatType;
    // TODO: 加载历史消息
}

void ChatWidget::onSendClicked() {
    // TODO: 发送消息
}

void ChatWidget::onFileClicked() {
    // TODO: 选择文件
}
