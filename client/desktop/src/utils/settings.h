#pragma once

#include <QString>

/**
 * 应用设置管理
 */
class Settings {
public:
    static Settings& instance();
    
    // 服务器配置
    QString serverUrl() const;
    void setServerUrl(const QString& url);
    
    // 用户信息
    QString savedUserId() const;
    QString savedToken() const;
    void saveLoginInfo(const QString& userId, const QString& token);
    void clearLoginInfo();
    
    // 界面设置
    bool rememberPassword() const;
    void setRememberPassword(bool remember);
    
private:
    Settings();
    ~Settings();
    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;
};
