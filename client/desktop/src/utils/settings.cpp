#include "settings.h"
#include <QSettings>

Settings& Settings::instance() {
    static Settings instance;
    return instance;
}

Settings::Settings() = default;
Settings::~Settings() = default;

QString Settings::serverUrl() const {
    QSettings settings;
    return settings.value("server/url", "ws://localhost:9090/ws").toString();
}

void Settings::setServerUrl(const QString& url) {
    QSettings settings;
    settings.setValue("server/url", url);
}

QString Settings::savedUserId() const {
    QSettings settings;
    return settings.value("user/userId").toString();
}

QString Settings::savedToken() const {
    QSettings settings;
    return settings.value("user/token").toString();
}

void Settings::saveLoginInfo(const QString& userId, const QString& token) {
    QSettings settings;
    settings.setValue("user/userId", userId);
    settings.setValue("user/token", token);
}

void Settings::clearLoginInfo() {
    QSettings settings;
    settings.remove("user/userId");
    settings.remove("user/token");
}

bool Settings::rememberPassword() const {
    QSettings settings;
    return settings.value("user/rememberPassword", false).toBool();
}

void Settings::setRememberPassword(bool remember) {
    QSettings settings;
    settings.setValue("user/rememberPassword", remember);
}
