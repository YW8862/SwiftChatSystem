#pragma once

#include <QWidget>

/**
 * 登录窗口
 */
class LoginWindow : public QWidget {
    Q_OBJECT
    
public:
    explicit LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow();
    
signals:
    void loginSuccess(const QString& userId, const QString& token);
    
private slots:
    void onLoginClicked();
    void onRegisterClicked();
    
private:
    // TODO: UI 组件
};
