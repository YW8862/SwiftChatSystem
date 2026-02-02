#pragma once

#include <QMainWindow>

/**
 * 主窗口
 * 
 * 布局：
 * - 左侧：联系人列表 / 会话列表
 * - 右侧：聊天区域
 * - 顶部：工具栏
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
private:
    // TODO: UI 组件
};
