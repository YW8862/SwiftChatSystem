#include "blacklistdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QInputDialog>

BlacklistDialog::BlacklistDialog(QWidget *parent)
    : QDialog(parent) {
    setupUI();
}

BlacklistDialog::~BlacklistDialog() = default;

void BlacklistDialog::setupUI() {
    setWindowTitle(QStringLiteral("黑名单管理"));
    resize(450, 500);
    
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setSpacing(12);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    
    // 标题栏
    auto* titleRow = new QHBoxLayout();
    m_titleLabel = new QLabel(QStringLiteral("黑名单列表"), this);
    m_titleLabel->setStyleSheet("font-size: 15px; font-weight: 600; color: #1f1f1f;");
    titleRow->addWidget(m_titleLabel);
    titleRow->addStretch();
    
    m_countLabel = new QLabel(QStringLiteral("0 人"), this);
    m_countLabel->setStyleSheet("color: #888; font-size: 13px;");
    titleRow->addWidget(m_countLabel);
    rootLayout->addLayout(titleRow);
    
    // 黑名单列表
    m_blockedList = new QListWidget(this);
    m_blockedList->setFrameShape(QFrame::NoFrame);
    m_blockedList->setStyleSheet(
        "QListWidget { "
        "background: #ffffff; border: 1px solid #e0e0e0; border-radius: 8px; "
        "padding: 4px; }"
        "QListWidget::item { "
        "padding: 10px 12px; border-radius: 6px; margin: 2px 0; "
        "background: #fafafa; }"
        "QListWidget::item:selected { "
        "background: #ffebee; color: #c62828; }"
    );
    rootLayout->addWidget(m_blockedList, 1);
    
    // 操作按钮
    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(8);
    
    m_addBtn = new QPushButton(QStringLiteral("添加黑名单"), this);
    m_addBtn->setCursor(Qt::PointingHandCursor);
    m_addBtn->setStyleSheet(
        "QPushButton { "
        "background: #ff5252; color: white; border: none; "
        "border-radius: 6px; padding: 8px 16px; font-size: 13px; }"
        "QPushButton:hover { background: #ff6666; }"
        "QPushButton:pressed { background: #e04040; }"
    );
    connect(m_addBtn, &QPushButton::clicked, this, &BlacklistDialog::onAddUserClicked);
    btnRow->addWidget(m_addBtn);
    
    m_removeBtn = new QPushButton(QStringLiteral("移除选中"), this);
    m_removeBtn->setCursor(Qt::PointingHandCursor);
    m_removeBtn->setEnabled(false);
    m_removeBtn->setStyleSheet(
        "QPushButton { "
        "background: #f0f0f0; color: #333; border: none; "
        "border-radius: 6px; padding: 8px 16px; font-size: 13px; }"
        "QPushButton:hover { background: #e0e0e0; }"
        "QPushButton:pressed { background: #d0d0d0; }"
        "QPushButton:disabled { background: #e8e8e8; color: #aaa; }"
    );
    connect(m_removeBtn, &QPushButton::clicked, this, &BlacklistDialog::onRemoveUserClicked);
    btnRow->addWidget(m_removeBtn);
    
    btnRow->addStretch();
    rootLayout->addLayout(btnRow);
    
    // 关闭按钮
    m_closeBtn = new QPushButton(QStringLiteral("关闭"), this);
    m_closeBtn->setFixedSize(100, 36);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setStyleSheet(
        "QPushButton { "
        "background: #f5f5f5; color: #333; border: none; "
        "border-radius: 6px; padding: 8px 16px; font-size: 14px; }"
        "QPushButton:hover { background: #e8e8e8; }"
        "QPushButton:pressed { background: #dcdcdc; }"
    );
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    
    auto* closeRow = new QHBoxLayout();
    closeRow->addStretch();
    closeRow->addWidget(m_closeBtn);
    rootLayout->addLayout(closeRow);
    
    // 连接列表选择事件
    connect(m_blockedList, &QListWidget::itemSelectionChanged, this, [this]() {
        m_removeBtn->setEnabled(m_blockedList->currentItem() != nullptr);
    });
    
    updateCount();
}

void BlacklistDialog::updateCount() {
    int count = m_blockedUsers.size();
    m_countLabel->setText(QString("%1 人").arg(count));
}

void BlacklistDialog::setBlockedUsers(const QMap<QString, QString>& blockedUsers) {
    m_blockedUsers = blockedUsers;
    refreshList();
    updateCount();
}

QMap<QString, QString> BlacklistDialog::getBlockedUsers() const {
    return m_blockedUsers;
}

void BlacklistDialog::refreshList() {
    m_blockedList->clear();
    
    for (auto it = m_blockedUsers.begin(); it != m_blockedUsers.end(); ++it) {
        QString userId = it.key();
        QString userName = it.value();
        
        auto* item = new QListWidgetItem(userName.isEmpty() ? userId : userName);
        item->setData(Qt::UserRole, userId);
        item->setIcon(QIcon::fromTheme("dialog-warning", QIcon()));
        
        // 设置红色警告图标
        if (!userName.isEmpty()) {
            item->setForeground(QBrush(QColor("#333")));
        } else {
            item->setForeground(QBrush(QColor("#666")));
        }
        
        m_blockedList->addItem(item);
    }
}

void BlacklistDialog::onAddUserClicked() {
    // 弹出对话框让用户输入要拉黑的用户 ID
    bool ok = false;
    QString userId = QInputDialog::getText(
        this,
        QStringLiteral("添加黑名单"),
        QStringLiteral("请输入要拉黑的用户 ID："),
        QLineEdit::Normal,
        QString(),
        &ok
    );
    
    if (!ok || userId.trimmed().isEmpty()) return;
    
    userId = userId.trimmed();
    
    // 检查是否已在黑名单中
    if (m_blockedUsers.contains(userId)) {
        QMessageBox::warning(
            this,
            QStringLiteral("提示"),
            QString(QStringLiteral("用户 %1 已在黑名单中。")).arg(userId)
        );
        return;
    }
    
    // 发出信号，由父组件处理实际的 RPC 调用
    emit addUserRequested(userId, userId);  // 临时用 userId 作为 userName
}

void BlacklistDialog::onRemoveUserClicked() {
    QListWidgetItem* currentItem = m_blockedList->currentItem();
    if (!currentItem) return;
    
    QString userId = currentItem->data(Qt::UserRole).toString();
    if (userId.isEmpty()) return;
    
    // 二次确认
    auto reply = QMessageBox::question(
        this,
        QStringLiteral("确认移除"),
        QString(QStringLiteral("确定要将 %1 从黑名单中移除吗？")).arg(
            currentItem->text()
        ),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply != QMessageBox::Yes) return;
    
    // 发出信号，由父组件处理实际的 RPC 调用
    emit removeUserRequested(userId);
    
    // 从列表中移除（实际数据由父组件更新）
    delete currentItem;
    updateCount();
}
