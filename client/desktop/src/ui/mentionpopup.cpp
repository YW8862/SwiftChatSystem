#include "mentionpopup.h"

#include <QVBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QFrame>
#include <QPoint>
#include <QApplication>
#include <QDesktopWidget>
#include <QKeyEvent>
#include <QTextEdit>

MentionPopupDialog::MentionPopupDialog(QWidget *parent)
    : QWidget(parent) {
    // 设置为无边框弹窗
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    
    setupUI();
}

MentionPopupDialog::~MentionPopupDialog() = default;

void MentionPopupDialog::setupUI() {
    // 设置固定大小
    setFixedSize(250, 300);
    
    // 弹窗容器
    m_popupFrame = new QFrame(this);
    m_popupFrame->setStyleSheet(
        "QFrame {"
        "   background: white;"
        "   border: 1px solid #e0e0e0;"
        "   border-radius: 6px;"
        "}"
    );
    auto* layout = new QVBoxLayout(m_popupFrame);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    // 提示标签
    m_hintLabel = new QLabel(QStringLiteral("选择要@的用户"), m_popupFrame);
    m_hintLabel->setStyleSheet(
        "QLabel {"
        "   background: #f5f5f5;"
        "   color: #666;"
        "   padding: 8px 12px;"
        "   font-size: 12px;"
        "   border-bottom: 1px solid #e0e0e0;"
        "}"
    );
    layout->addWidget(m_hintLabel);
    
    // 成员列表
    m_memberList = new QListWidget(m_popupFrame);
    m_memberList->setStyleSheet(
        "QListWidget {"
        "   border: none;"
        "   outline: none;"
        "   font-size: 13px;"
        "}"
        "QListWidget::item {"
        "   padding: 8px 12px;"
        "   border-bottom: 1px solid #f0f0f0;"
        "}"
        "QListWidget::item:hover {"
        "   background: #f0f7ff;"
        "}"
        "QListWidget::item:selected {"
        "   background: #e6f3ff;"
        "   color: #3498db;"
        "}"
    );
    m_memberList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_memberList->installEventFilter(this);
    layout->addWidget(m_memberList);
    
    connect(m_memberList, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        onMemberClicked(item->data(Qt::UserRole).toString());
    });
    connect(m_memberList, &QListWidget::currentRowChanged, this, &MentionPopupDialog::onCurrentRowChanged);
}

void MentionPopupDialog::setMembers(const QStringList& members, const QMap<QString, QString>& memberNames) {
    m_members = members;
    m_memberNames = memberNames;
    
    m_memberList->clear();
    
    for (const QString& userId : members) {
        QString userName = memberNames.value(userId, userId);
        
        auto* item = new QListWidgetItem(userName);
        item->setData(Qt::UserRole, userId);
        item->setData(Qt::DisplayRole, userName);
        
        m_memberList->addItem(item);
    }
    
    // 默认选中第一个
    if (m_memberList->count() > 0) {
        m_memberList->setCurrentRow(0);
    }
}

void MentionPopupDialog::showAt(const QPoint& position) {
    move(position);
    show();
    raise();
    setFocus();
}

void MentionPopupDialog::setInputContext(QTextEdit* input, const QString& currentText, int cursorPosition) {
    m_inputEditor = input;
    m_cursorPosition = cursorPosition;
    
    // 查找@符号的位置
    int atPos = -1;
    for (int i = cursorPosition - 1; i >= 0; --i) {
        if (currentText[i] == '@') {
            atPos = i;
            break;
        } else if (currentText[i] == ' ' || currentText[i] == '\n') {
            break;
        }
    }
    
    m_mentionStartPos = atPos;
}

QString MentionPopupDialog::selectedUserId() const {
    return m_selectedUserId;
}

QString MentionPopupDialog::selectedUserName() const {
    return m_selectedUserName;
}

void MentionPopupDialog::filterMembers(const QString& searchText) {
    m_memberList->clear();
    
    QString filter = searchText.toLower();
    
    for (const QString& userId : m_members) {
        QString userName = m_memberNames.value(userId, userId);
        
        if (userName.contains(filter, Qt::CaseInsensitive)) {
            auto* item = new QListWidgetItem(userName);
            item->setData(Qt::UserRole, userId);
            m_memberList->addItem(item);
        }
    }
    
    // 如果没有匹配项，隐藏弹窗
    if (m_memberList->count() == 0) {
        hide();
    } else {
        // 默认选中第一个
        if (m_memberList->count() > 0) {
            m_memberList->setCurrentRow(0);
        }
    }
}

bool MentionPopupDialog::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_memberList) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            
            switch (keyEvent->key()) {
                case Qt::Key_Return:
                case Qt::Key_Enter:
                    if (m_memberList->currentItem()) {
                        onMemberClicked(m_memberList->currentItem()->data(Qt::UserRole).toString());
                    }
                    return true;
                    
                case Qt::Key_Escape:
                    emit canceled();
                    hide();
                    return true;
            }
        }
    }
    
    return QWidget::eventFilter(obj, event);
}

void MentionPopupDialog::hideEvent(QHideEvent* event) {
    QWidget::hideEvent(event);
    emit canceled();
}

void MentionPopupDialog::onMemberClicked(const QString& userId) {
    selectUser(userId, m_memberNames.value(userId, userId));
}

void MentionPopupDialog::onCurrentRowChanged(int currentRow) {
    if (currentRow >= 0 && currentRow < m_memberList->count()) {
        auto* item = m_memberList->item(currentRow);
        if (item) {
            m_memberList->scrollToItem(item);
        }
    }
}

void MentionPopupDialog::selectUser(const QString& userId, const QString& userName) {
    m_selectedUserId = userId;
    m_selectedUserName = userName;
    
    // 在输入框中替换@xxx 为@用户名
    if (m_inputEditor && m_mentionStartPos >= 0) {
        QTextCursor cursor = m_inputEditor->textCursor();
        int originalPos = cursor.position();
        
        // 删除@后面的内容
        cursor.setPosition(m_mentionStartPos + 1);
        cursor.setPosition(originalPos, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        
        // 插入用户名
        cursor.insertText(userName);
        
        // 添加空格
        cursor.insertText(" ");
    }
    
    emit userSelected(userId, userName);
    hide();
}
