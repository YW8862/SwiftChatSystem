#pragma once

#include <QWidget>

/**
 * 联系人/会话列表组件
 */
class ContactWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit ContactWidget(QWidget *parent = nullptr);
    ~ContactWidget();
    
signals:
    void conversationSelected(const QString& chatId, int chatType);
    void friendSelected(const QString& userId);
    
private:
    // TODO: UI 组件
};
