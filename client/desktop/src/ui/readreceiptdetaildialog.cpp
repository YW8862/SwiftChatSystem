#include "readreceiptdetaildialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGroupBox>

ReadReceiptDetailDialog::ReadReceiptDetailDialog(QWidget *parent)
    : QDialog(parent) {
    setupUI();
}

ReadReceiptDetailDialog::~ReadReceiptDetailDialog() = default;

void ReadReceiptDetailDialog::setupUI() {
    setWindowTitle(QStringLiteral("已读详情"));
    resize(400, 500);
    
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setSpacing(12);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    
    // 标题
    m_titleLabel = new QLabel(QStringLiteral("已读：0 / 未读：0"), this);
    m_titleLabel->setStyleSheet("font-size: 14px; font-weight: 600; color: #1f1f1f;");
    rootLayout->addWidget(m_titleLabel);
    
    // 已读列表
    auto* readGroup = new QGroupBox(QStringLiteral("已读成员"), this);
    readGroup->setStyleSheet(
        "QGroupBox { "
        "font-weight: 600; color: #07c160; "
        "border: 1px solid #07c160; border-radius: 6px; "
        "margin-top: 8px; padding-top: 8px; "
        "}"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; }"
    );
    auto* readLayout = new QVBoxLayout(readGroup);
    readLayout->setContentsMargins(8, 20, 8, 8);
    
    m_readList = new QListWidget(readGroup);
    m_readList->setFrameShape(QFrame::NoFrame);
    m_readList->setStyleSheet(
        "QListWidget { background: transparent; border: none; }"
        "QListWidget::item { padding: 6px 8px; border-radius: 4px; }"
        "QListWidget::item:selected { background: #e8f5e9; color: #07c160; }"
    );
    readLayout->addWidget(m_readList);
    rootLayout->addWidget(readGroup);
    
    // 未读列表（群聊时显示）
    auto* unreadGroup = new QGroupBox(QStringLiteral("未读成员"), this);
    unreadGroup->setStyleSheet(
        "QGroupBox { "
        "font-weight: 600; color: #ff9800; "
        "border: 1px solid #ff9800; border-radius: 6px; "
        "margin-top: 8px; padding-top: 8px; "
        "}"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; }"
    );
    auto* unreadLayout = new QVBoxLayout(unreadGroup);
    unreadLayout->setContentsMargins(8, 20, 8, 8);
    
    m_unreadList = new QListWidget(unreadGroup);
    m_unreadList->setFrameShape(QFrame::NoFrame);
    m_unreadList->setStyleSheet(
        "QListWidget { background: transparent; border: none; }"
        "QListWidget::item { padding: 6px 8px; border-radius: 4px; }"
        "QListWidget::item:selected { background: #fff3e0; color: #ff9800; }"
    );
    unreadLayout->addWidget(m_unreadList);
    rootLayout->addWidget(unreadGroup);
    
    // 关闭按钮
    auto* closeBtn = new QPushButton(QStringLiteral("关闭"), this);
    closeBtn->setFixedSize(100, 36);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(
        "QPushButton { background: #f0f0f0; color: #333; border: none; border-radius: 6px; font-size: 14px; }"
        "QPushButton:hover { background: #e0e0e0; }"
        "QPushButton:pressed { background: #d0d0d0; }"
    );
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    btnLayout->addStretch();
    rootLayout->addLayout(btnLayout);
}

void ReadReceiptDetailDialog::updateTitle() {
    if (m_unreadCount > 0) {
        m_titleLabel->setText(QString("已读：%1 / 未读：%2").arg(m_readCount).arg(m_unreadCount));
    } else {
        m_titleLabel->setText(QString("已读：%1").arg(m_readCount));
    }
}

void ReadReceiptDetailDialog::setPrivateChatReceipt(const QString& readerName, bool hasRead) {
    m_readList->clear();
    m_unreadList->clear();
    
    if (hasRead) {
        m_readList->addItem(readerName);
        m_readCount = 1;
        m_unreadCount = 0;
    } else {
        m_unreadList->addItem(readerName);
        m_readCount = 0;
        m_unreadCount = 1;
    }
    
    updateTitle();
}

void ReadReceiptDetailDialog::setGroupChatReceipt(const QMap<QString, QString>& readUsers,
                                                   const QMap<QString, QString>& unreadUsers) {
    m_readList->clear();
    m_unreadList->clear();
    
    // 添加已读用户
    for (auto it = readUsers.begin(); it != readUsers.end(); ++it) {
        QString userId = it.key();
        QString userName = it.value();
        m_readList->addItem(userName.isEmpty() ? userId : userName);
    }
    m_readCount = readUsers.size();
    
    // 添加未读用户
    for (auto it = unreadUsers.begin(); it != unreadUsers.end(); ++it) {
        QString userId = it.key();
        QString userName = it.value();
        m_unreadList->addItem(userName.isEmpty() ? userId : userName);
    }
    m_unreadCount = unreadUsers.size();
    
    updateTitle();
}
