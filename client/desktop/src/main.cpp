/**
 * SwiftChat Desktop Client
 * 
 * Qt5 桌面客户端入口
 */

#include <QApplication>
// #include "ui/loginwindow.h"
// #include "ui/mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    app.setApplicationName("SwiftChat");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Swift");
    
    // TODO: 显示登录窗口
    // LoginWindow loginWindow;
    // loginWindow.show();
    
    return app.exec();
}
