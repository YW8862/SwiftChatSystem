#pragma once

#include <QDialog>
#include <QString>
#include <QMap>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>

/**
 * 黑名单管理对话框
 * 
 * 功能：
 * - 显示黑名单用户列表
 * - 添加用户到黑名单
 * - 从黑名单移除用户
 */
class BlacklistDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit BlacklistDialog(QWidget *parent = nullptr);
    ~BlacklistDialog();
    
    /**
     * 设置黑名单用户列表
     * @param blockedUsers 黑名单用户映射 (userId -> userName)
     */
    void setBlockedUsers(const QMap<QString, QString>& blockedUsers);
    
    /**
     * 获取当前黑名单用户列表
     */
    QMap<QString, QString> getBlockedUsers() const;
    
signals:
    /**
     * 用户请求添加黑名单
     * @param userId 用户 ID
     * @param userName 用户名称
     */
    void addUserRequested(const QString& userId, const QString& userName);
    
    /**
     * 用户请求移除黑名单
     * @param userId 用户 ID
     */
    void removeUserRequested(const QString& userId);
    
private slots:
    void onAddUserClicked();
    void onRemoveUserClicked();
    void refreshList();
    
private:
    void setupUI();
    void updateCount();
    
    QLabel* m_titleLabel = nullptr;
    QLabel* m_countLabel = nullptr;
    QListWidget* m_blockedList = nullptr;
    QPushButton* m_addBtn = nullptr;
    QPushButton* m_removeBtn = nullptr;
    QPushButton* m_closeBtn = nullptr;
    
    QMap<QString, QString> m_blockedUsers;  // userId -> userName
};
