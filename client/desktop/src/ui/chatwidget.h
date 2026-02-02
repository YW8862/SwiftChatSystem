#pragma once

#include <QWidget>

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
    
signals:
    void messageSent(const QString& content);
    void fileSelected(const QString& filePath);
    
private slots:
    void onSendClicked();
    void onFileClicked();
    
private:
    QString m_chatId;
    int m_chatType;  // 1=私聊, 2=群聊
};
