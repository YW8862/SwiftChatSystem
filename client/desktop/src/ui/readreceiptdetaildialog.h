#pragma once

#include <QDialog>
#include <QString>
#include <QMap>
#include <QListWidget>
#include <QLabel>

/**
 * 已读详情对话框
 * 
 * 功能：
 * - 显示已读成员列表
 * - 显示未读成员列表（群聊）
 */
class ReadReceiptDetailDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit ReadReceiptDetailDialog(QWidget *parent = nullptr);
    ~ReadReceiptDetailDialog();
    
    /**
     * 设置私聊已读信息
     */
    void setPrivateChatReceipt(const QString& readerName, bool hasRead);
    
    /**
     * 设置群聊已读信息
     * @param readUsers 已读用户列表 (userId -> userName)
     * @param unreadUsers 未读用户列表 (userId -> userName)
     */
    void setGroupChatReceipt(const QMap<QString, QString>& readUsers,
                            const QMap<QString, QString>& unreadUsers);
    
private:
    void setupUI();
    void updateTitle();
    
    QLabel* m_titleLabel = nullptr;
    QListWidget* m_readList = nullptr;
    QListWidget* m_unreadList = nullptr;
    
    int m_readCount = 0;
    int m_unreadCount = 0;
};
