#pragma once

#include <QWidget>

/**
 * 消息气泡组件
 */
class MessageItem : public QWidget {
    Q_OBJECT
    
public:
    explicit MessageItem(QWidget *parent = nullptr);
    ~MessageItem();
    
    void setMessage(const QString& msgId, const QString& content, 
                    const QString& senderName, bool isSelf, qint64 timestamp);
    void setRecalled(bool recalled);
    
private:
    QString m_msgId;
    bool m_isSelf;
};
