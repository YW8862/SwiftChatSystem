#include "messageitem.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QPixmap>
#include <QFileDialog>
#include <QApplication>
#include <QScreen>
#include <QCursor>
#include <QDateTime>
#include <QMenu>
#include <QContextMenuEvent>
#include "../utils/imageloader.h"
#include "filemessageitem.h"

MessageItem::MessageItem(QWidget *parent) : QWidget(parent) {
    setupUI();
}

MessageItem::~MessageItem() = default;

void MessageItem::setupUI() {
    auto* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 4, 0, 4);
    rootLayout->setSpacing(8);
    
    // 头像
    m_avatarLabel = new QLabel(this);
    m_avatarLabel->setFixedSize(40, 40);
    m_avatarLabel->setStyleSheet(
        "QLabel { background: #e0e0e0; border-radius: 20px; }"
    );
    m_avatarLabel->setAlignment(Qt::AlignCenter);
    m_avatarLabel->setText(QStringLiteral("👤"));
    
    // 消息气泡区域
    auto* bubbleContainer = new QWidget();
    m_bubbleLayout = new QVBoxLayout(bubbleContainer);
    m_bubbleLayout->setContentsMargins(0, 0, 0, 0);
    m_bubbleLayout->setSpacing(4);
    
    // 昵称（群聊时显示）
    m_nameLabel = new QLabel(bubbleContainer);
    m_nameLabel->setStyleSheet("color: #888; font-size: 12px; padding-left: 4px;");
    m_nameLabel->setVisible(false);  // 私聊时隐藏
    m_bubbleLayout->addWidget(m_nameLabel);
    
    // 引用消息容器
    m_replyFrame = new QFrame(bubbleContainer);
    m_replyFrame->setObjectName("replyBubble");
    m_replyFrame->setStyleSheet(
        "QFrame#replyBubble { background: rgba(0, 0, 0, 0.05); border-left: 3px solid #07c160; "
        "border-radius: 4px; padding: 6px 8px; margin: 2px 0; }"
    );
    m_replyFrame->setVisible(false);
    auto* replyLayout = new QVBoxLayout(m_replyFrame);
    replyLayout->setContentsMargins(0, 0, 0, 0);
    replyLayout->setSpacing(2);
    
    m_replyLabel = new QLabel(m_replyFrame);
    m_replyLabel->setWordWrap(true);
    m_replyLabel->setStyleSheet("color: #666; font-size: 13px; padding-left: 6px;");
    m_replyLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_replyLabel->setCursor(Qt::PointingHandCursor);
    // 安装事件过滤器以支持点击跳转
    m_replyLabel->installEventFilter(this);
    replyLayout->addWidget(m_replyLabel);
    m_bubbleLayout->addWidget(m_replyFrame);
    
    // 消息气泡
    m_bubbleFrame = new QFrame(bubbleContainer);
    m_bubbleFrame->setObjectName("messageBubble");
    m_bubbleFrame->setStyleSheet(
        "QFrame#messageBubble { background: #ffffff; border-radius: 8px; padding: 8px 10px; }"
    );
    
    auto* contentLayout = new QVBoxLayout(m_bubbleFrame);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(6);
    
    // 图片标签（如果是图片消息）- 使用 ImageLoader 替代
    m_imageLoader = new ImageLoader(m_bubbleFrame);
    m_imageLoader->setMaxSize(300, 300);
    m_imageLoader->setVisible(false);
    connect(m_imageLoader, &ImageLoader::loadFinished, this, [this](const QPixmap& pixmap) {
        Q_UNUSED(pixmap);
        // 加载完成，可以在这里更新 tooltip 等
        if (m_imageLoader->property("isAnimated").toBool()) {
            m_imageLoader->setToolTip(QStringLiteral("GIF 动图 - 点击查看大图"));
        } else {
            m_imageLoader->setToolTip(QStringLiteral("点击查看大图"));
        }
    });
    connect(m_imageLoader, &ImageLoader::loadFailed, this, [this](const QString& error) {
        Q_UNUSED(error);
        // 加载失败，显示错误提示
    });
    
    // 文件消息项
    m_fileMessageItem = new FileMessageItem(m_bubbleFrame);
    m_fileMessageItem->setVisible(false);
    connect(m_fileMessageItem, &FileMessageItem::downloadRequested, this, [this](const QString& filePath) {
        emit fileDownloadRequested(filePath);  // 转发给父组件
    });
    connect(m_fileMessageItem, &FileMessageItem::openFileRequested, this, [this](const QString& filePath) {
        emit fileOpenRequested(filePath);  // 转发给父组件
    });
    
    // 内容标签（文本或文件信息）
    m_contentLabel = new QLabel(m_bubbleFrame);
    m_contentLabel->setWordWrap(true);
    m_contentLabel->setStyleSheet("color: #1f1f1f; font-size: 14px;");
    m_contentLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    contentLayout->addWidget(m_contentLabel);
    contentLayout->addWidget(m_imageLoader);
    contentLayout->addWidget(m_fileMessageItem);
    
    m_bubbleLayout->addWidget(m_bubbleFrame);
    rootLayout->addWidget(bubbleContainer);
    
    // 状态标签和重发按钮
    auto* statusLayout = new QVBoxLayout();
    statusLayout->setContentsMargins(4, 0, 0, 0);
    statusLayout->setSpacing(2);
    
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: #ff4d4f; font-size: 11px;");
    m_statusLabel->setVisible(false);
    statusLayout->addWidget(m_statusLabel);
    
    m_retryButton = new QPushButton(QStringLiteral("⚠ 重发"), this);
    m_retryButton->setFixedSize(50, 22);
    m_retryButton->setStyleSheet(
        "QPushButton { background: #ff4d4f; color: white; border: none; border-radius: 4px; font-size: 11px; }"
        "QPushButton:hover { background: #ff7875; }"
    );
    m_retryButton->setVisible(false);
    connect(m_retryButton, &QPushButton::clicked, this, &MessageItem::onRetryButtonClicked);
    statusLayout->addWidget(m_retryButton);
    
    rootLayout->addLayout(statusLayout);
    
    // 时间标签
    m_timeLabel = new QLabel(this);
    m_timeLabel->setStyleSheet("color: #b8b8b8; font-size: 11px; min-width: 50px;");
    m_timeLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    rootLayout->addWidget(m_timeLabel);
    
    updateMessageStyle();
}

void MessageItem::setMessage(const QString& msgId, const QString& content,
                              const QString& senderName, bool isSelf, qint64 timestamp,
                              const QString& mediaUrl, const QString& mediaType,
                              const QStringList& mentions,
                              const QString& replyToMsgId, 
                              const QString& replyToContent,
                              const QString& replyToSender) {
    m_msgId = msgId;
    m_senderName = senderName;
    m_isSelf = isSelf;
    m_mediaUrl = mediaUrl;
    m_mediaType = mediaType;
    m_mentions = mentions;  // 保存@列表
    m_replyToMsgId = replyToMsgId;
    m_replyToContent = replyToContent;
    m_replyToSender = replyToSender;
    
    // 更新昵称
    if (m_nameLabel) {
        m_nameLabel->setText(senderName);
        m_nameLabel->setVisible(!senderName.isEmpty());  // 群聊时显示昵称
    }
    
    // 更新头像（这里使用默认头像，实际项目中可从缓存加载）
    if (m_avatarLabel) {
        m_avatarLabel->setText(isSelf ? QStringLiteral("🧑") : QStringLiteral("👤"));
    }
    
    // 更新时间
    if (m_timeLabel && timestamp > 0) {
        m_timeLabel->setText(QDateTime::fromMSecsSinceEpoch(timestamp).toString("HH:mm"));
    }
    
    // 根据媒体类型设置内容
    if (mediaType == "image" && !mediaUrl.isEmpty()) {
        // 图片消息：使用 ImageLoader 异步加载（支持 GIF/WebP）
        if (m_contentLabel) m_contentLabel->hide();
        if (m_imageLoader) {
            m_imageLoader->show();
            m_imageLoader->loadImageAsync(mediaUrl, true, true);  // 支持动画
            
            // 添加点击事件（点击图片打开大图）
            m_imageLoader->setCursor(Qt::PointingHandCursor);
            
            // 安装事件过滤器以捕获鼠标点击
            m_imageLoader->installEventFilter(this);
        }
    } else if (mediaType == "file" && !mediaUrl.isEmpty()) {
        // 文件消息：使用 FileMessageItem 组件
        if (m_contentLabel) m_contentLabel->hide();
        if (m_imageLoader) m_imageLoader->hide();
        if (m_fileMessageItem) {
            m_fileMessageItem->show();
            
            // 解析文件信息
            QFileInfo fileInfo(mediaUrl);
            QString fileName = fileInfo.fileName();
            qint64 fileSize = fileInfo.size();
            
            // 设置文件信息（这里假设文件大小从 content 中解析）
            // 实际使用时应该从消息数据中获取
            m_fileMessageItem->setFileInfo(fileName, fileSize, mediaUrl, isSelf);
        }
    } else {
        // 文本消息
        if (m_imageLoader) m_imageLoader->hide();
        if (m_fileMessageItem) m_fileMessageItem->hide();
        if (m_contentLabel) {
            m_contentLabel->show();
            
            // 处理@高亮显示
            if (!mentions.isEmpty()) {
                // 使用富文本显示，高亮@的用户
                QString highlightedContent = content;
                
                // 这里简化处理，实际应该根据 mentions 列表找到对应的用户名并高亮
                // 示例：将@username 替换为带样式的 HTML
                for (const QString& userId : mentions) {
                    // TODO: 需要从某处获取 userId 对应的用户名
                    QString mentionText = "@" + userId;  // 临时用 userId 代替
                    QString highlightedMention = QString("<span style='color: #3498db; font-weight: 600;'>%1</span>").arg(mentionText);
                    highlightedContent.replace(mentionText, highlightedMention);
                }
                
                m_contentLabel->setText(highlightedContent);
                m_contentLabel->setTextFormat(Qt::RichText);
            } else {
                m_contentLabel->setText(content);
                m_contentLabel->setTextFormat(Qt::PlainText);
            }
        }
    }
    
    // 更新引用消息显示
    if (!replyToMsgId.isEmpty()) {
        setupReplyView(replyToMsgId, replyToContent, replyToSender);
    } else {
        if (m_replyFrame) m_replyFrame->hide();
    }
    
    updateMessageStyle();
}

void MessageItem::setRecalled(bool recalled) {
    m_recalled = recalled;
    if (recalled) {
        if (m_contentLabel) {
            m_contentLabel->show();
            m_contentLabel->setText(QStringLiteral("这条消息已撤回"));
            m_contentLabel->setStyleSheet("color: #999; font-size: 13px; font-style: italic;");
        }
        if (m_imageLoader) m_imageLoader->hide();
        if (m_fileMessageItem) m_fileMessageItem->hide();
        if (m_statusLabel) m_statusLabel->hide();
        if (m_retryButton) m_retryButton->hide();
    }
}

void MessageItem::setSendingState(bool sending) {
    m_sending = sending;
    if (m_statusLabel) {
        m_statusLabel->setVisible(sending);
        m_statusLabel->setText(QStringLiteral("发送中..."));
        m_statusLabel->setStyleSheet("color: #999; font-size: 11px;");
    }
    if (m_retryButton) m_retryButton->setVisible(false);
}

void MessageItem::setFailedState(bool failed) {
    m_sendFailed = failed;
    if (m_statusLabel) {
        m_statusLabel->setVisible(failed);
        m_statusLabel->setText(QStringLiteral("发送失败"));
        m_statusLabel->setStyleSheet("color: #ff4d4f; font-size: 11px;");
    }
    if (m_retryButton) {
        m_retryButton->setVisible(failed);
    }
}

void MessageItem::updateMessageStyle() {
    if (!m_bubbleFrame) return;
    
    if (m_recalled) {
        m_bubbleFrame->setStyleSheet(
            "QFrame#messageBubble { background: transparent; border: 1px dashed #ccc; border-radius: 8px; padding: 8px 10px; }"
        );
        return;
    }
    
    // 根据是自己/他人的消息，设置不同的样式和布局
    if (m_isSelf) {
        // 自己的消息：绿色气泡，靠右显示
        m_bubbleFrame->setStyleSheet(
            "QFrame#messageBubble { background: #95ec69; border-radius: 8px; padding: 8px 10px; }"
        );
        // 重新组织布局：头像在右，气泡靠右
        auto* rootLayout = qobject_cast<QHBoxLayout*>(layout());
        if (rootLayout) {
            rootLayout->insertWidget(0, m_avatarLabel);  // 头像放最右边
            rootLayout->setDirection(QBoxLayout::RightToLeft);
        }
    } else {
        // 他人的消息：白色气泡，靠左显示
        m_bubbleFrame->setStyleSheet(
            "QFrame#messageBubble { background: #ffffff; border-radius: 8px; padding: 8px 10px; }"
        );
        // 重新组织布局：头像在左，气泡靠左
        auto* rootLayout = qobject_cast<QHBoxLayout*>(layout());
        if (rootLayout) {
            rootLayout->insertWidget(0, m_avatarLabel);  // 头像放最左边
            rootLayout->setDirection(QBoxLayout::LeftToRight);
        }
    }
}

void MessageItem::setupReplyView(const QString& replyToMsgId, const QString& replyToContent, 
                                  const QString& replyToSender) {
    if (!m_replyFrame || !m_replyLabel) return;
    
    if (replyToMsgId.isEmpty()) {
        m_replyFrame->hide();
        return;
    }
    
    m_replyFrame->show();
    
    // 构建引用显示文本
    QString displayText;
    if (!replyToSender.isEmpty()) {
        displayText = QString("回复 %1：%2").arg(replyToSender, replyToContent);
    } else {
        displayText = QString("回复：%1").arg(replyToContent);
    }
    
    m_replyLabel->setText(displayText);
    
    // 存储原消息 ID 以便点击时跳转
    m_replyLabel->setProperty("targetMsgId", replyToMsgId);
}

void MessageItem::onReplyActionTriggered() {
    // 发出回复信号，由父组件处理
    emit replyRequested(m_msgId, m_contentLabel ? m_contentLabel->text() : "",
                       m_senderName, m_msgId);  // 这里用 msgId 临时作为 senderId
}

void MessageItem::onImageLabelClicked() {
    if (m_mediaUrl.isEmpty() || m_mediaType != "image") return;
    
    // 发出信号，由父组件处理显示大图
    emit imageClicked(m_msgId, m_mediaUrl);
}

void MessageItem::onRetryButtonClicked() {
    emit retrySendRequested(m_msgId);
}

// 重写 eventFilter 以支持点击图片和引用消息
bool MessageItem::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_imageLoader && event->type() == QEvent::MouseButtonRelease) {
        onImageLabelClicked();
        return true;
    }
    
    // 处理引用消息的点击事件
    if (obj == m_replyLabel && event->type() == QEvent::MouseButtonRelease) {
        QString targetMsgId = m_replyLabel->property("targetMsgId").toString();
        if (!targetMsgId.isEmpty()) {
            // 发出信号，请求跳转到目标消息
            emit replyRequested(targetMsgId, QString(), QString(), QString());
        }
        return true;
    }
    
    return QWidget::eventFilter(obj, event);
}

// 右键菜单事件
void MessageItem::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);
    
    // 添加"回复"选项
    QAction* replyAction = menu.addAction(QStringLiteral("回复"));
    connect(replyAction, &QAction::triggered, this, &MessageItem::onReplyActionTriggered);
    
    // 如果是自己发送的消息，添加撤回选项
    if (m_isSelf && !m_recalled) {
        QAction* recallAction = menu.addAction(QStringLiteral("撤回"));
        connect(recallAction, &QAction::triggered, this, [this]() {
            // 这里可以发出撤回信号，暂时不实现
        });
    }
    
    menu.exec(event->globalPos());
}
