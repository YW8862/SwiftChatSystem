#include "client_logger.h"

#include <QDir>
#include <QStandardPaths>
#include <QtGlobal>

#include <swift/log_helper.h>

namespace client::logger {

namespace {

QtMessageHandler g_prev_handler = nullptr;
bool g_logger_ready = false;

void QtToSwiftLogHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    const std::string file = context.file ? context.file : "";
    const std::string func = context.function ? context.function : "";
    const int line = context.line;
    const std::string text = msg.toStdString();

    switch (type) {
        case QtDebugMsg:
            LogDebug("[Qt] " << text << " (" << file << ":" << line << ", " << func << ")");
            break;
        case QtInfoMsg:
            LogInfo("[Qt] " << text << " (" << file << ":" << line << ", " << func << ")");
            break;
        case QtWarningMsg:
            LogWarning("[Qt] " << text << " (" << file << ":" << line << ", " << func << ")");
            break;
        case QtCriticalMsg:
            LogError("[Qt] " << text << " (" << file << ":" << line << ", " << func << ")");
            break;
        case QtFatalMsg:
            LogFatal("[Qt] " << text << " (" << file << ":" << line << ", " << func << ")");
            break;
    }
}

}  // namespace

QString LogDirectory() {
    const QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (baseDir.isEmpty()) {
        return "./logs";
    }
    return baseDir + "/logs";
}

bool Init() {
    const QString logDir = LogDirectory();
    QDir().mkpath(logDir);

    swift::log::Config config;
    config.log_dir = logDir.toStdString();
    config.file_prefix = "swiftchat-client";
    config.enable_console = true;
    config.enable_file = true;
    config.show_file_line = true;
    config.console_color = true;
#ifdef NDEBUG
    config.min_level = swift::log::Level::INFO;
#else
    config.min_level = swift::log::Level::DEBUG;
#endif

    g_logger_ready = swift::log::Init(config);
    if (!g_logger_ready) {
        return false;
    }

    g_prev_handler = qInstallMessageHandler(QtToSwiftLogHandler);
    LogInfo("Client logger initialized, log_dir=" << config.log_dir);
    return true;
}

void Shutdown() {
    if (!g_logger_ready) {
        return;
    }
    LogInfo("Client logger shutting down");
    qInstallMessageHandler(g_prev_handler);
    swift::log::Shutdown();
    g_logger_ready = false;
    g_prev_handler = nullptr;
}

}  // namespace client::logger
