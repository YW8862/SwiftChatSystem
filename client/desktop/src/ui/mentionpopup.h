#pragma once

#include <QWidget>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QListWidget>
#include <QLabel>

class QTextEdit;
class QVBoxLayout;
class QFrame;

/**
 * @提醒用户选择弹窗
 * 
 * 功能：
 * - 显示群成员列表
 * - 支持搜索过滤
 * - 点击选择用户
 * - 键盘上下键导航
 */
class MentionPopupDialog : public QWidget {
    Q_OBJECT
    
public:
    explicit MentionPopupDialog(QWidget *parent = nullptr);
    ~MentionPopupDialog();
    
    /**
     * 设置成员列表
     * @param members 成员 ID 列表
     * @param memberNames 成员名称映射（userId -> userName）
     */
    void setMembers(const QStringList& members, const QMap<QString, QString>& memberNames);
    
    /**
     * 显示在指定位置
     */
    void showAt(const QPoint& position);
    
    /**
     * 设置当前输入的文本和光标位置（用于定位）
     */
    void setInputContext(QTextEdit* input, const QString& currentText, int cursorPosition);
    
    /**
     * 获取选中的用户 ID
     */
    QString selectedUserId() const;
    
    /**
     * 获取选中的用户名
     */
    QString selectedUserName() const;
    
    /**
     * 过滤成员列表（根据输入内容）
     */
    void filterMembers(const QString& searchText);
    
protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    
signals:
    /**
     * 用户选中信号
     * @param userId 用户 ID
     * @param userName 用户名
     */
    void userSelected(const QString& userId, const QString& userName);
    
    /**
     * 取消选择信号
     */
    void canceled();
    
private slots:
    void onMemberClicked(const QString& userId);
    void onCurrentRowChanged(int currentRow);
    
private:
    void setupUI();
    void updatePosition();
    void selectUser(const QString& userId, const QString& userName);
    
    // UI 组件
    QFrame* m_popupFrame;         // 弹窗容器
    QListWidget* m_memberList;    // 成员列表
    QLabel* m_hintLabel;          // 提示标签
    
    // 数据
    QStringList m_members;
    QMap<QString, QString> m_memberNames;
    QString m_selectedUserId;
    QString m_selectedUserName;
    QTextEdit* m_inputEditor = nullptr;
    int m_cursorPosition = 0;
    int m_mentionStartPos = 0;  // @符号的位置
    
    // 样式
    QString normalStyle;
    QString hoveredStyle;
};
