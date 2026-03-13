#pragma once

#include <QWidget>
#include <QString>
#include <QLabel>
#include <QPushButton>

class QFrame;
class QVBoxLayout;
class QHBoxLayout;
class ImageLoader;
class FileMessageItem;

/**
 * 消息气泡组件
 * 
 * 功能：
 * - 显示文本消息
 * - 显示图片消息（缩略图 + 点击放大）
 * - 显示文件消息
 * - 显示撤回状态
 * - 区分自己/他人的消息样式
 */
class MessageItem : public QWidget {
    Q_OBJECT
    
public:
    explicit MessageItem(QWidget *parent = nullptr);
    ~MessageItem();
    
    /**
     * 设置消息内容
     * @param msgId 消息 ID
     * @param content 消息内容
     * @param senderName 发送者名称
     * @param isSelf 是否是自己发送的
     * @param timestamp 时间戳
     * @param mediaUrl 媒体文件 URL（图片/文件等）
     * @param mediaType 媒体类型（text/image/file/voice/video）
     * @param mentions 被@的用户 ID 列表
     * @param replyToMsgId 回复的消息 ID
     * @param replyToContent 回复的消息内容
     * @param replyToSender 回复的消息发送者
     */
    void setMessage(const QString& msgId, const QString& content, 
                    const QString& senderName, bool isSelf, qint64 timestamp,
                    const QString& mediaUrl = QString(), const QString& mediaType = "text",
                    const QStringList& mentions = QStringList(),
                    const QString& replyToMsgId = QString(), 
                    const QString& replyToContent = QString(),
                    const QString& replyToSender = QString());
    
    /**
     * 设置撤回状态
     */
    void setRecalled(bool recalled);
    
    /**
     * 设置发送状态
     */
    void setSendingState(bool sending);
    
    /**
     * 设置发送失败状态
     */
    void setFailedState(bool failed);
    
signals:
    /**
     * 点击图片消息时发出信号
     */
    void imageClicked(const QString& msgId, const QString& imageUrl);
    
    /**
     * 请求重新发送失败的消息
     */
    void retrySendRequested(const QString& msgId);
    
    /**
     * 请求下载文件
     */
    void fileDownloadRequested(const QString& filePath);
    
    /**
     * 请求打开文件
     */
    void fileOpenRequested(const QString& filePath);
    
    /**
     * 请求回复消息
     */
    void replyRequested(const QString& msgId, const QString& content, 
                       const QString& senderName, const QString& senderId);
    
private slots:
    void onImageLabelClicked();
    void onRetryButtonClicked();
    void onReplyActionTriggered();
    
protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    
private:
    void setupUI();
    void updateMessageStyle();
    void showImagePreview(const QString& imageUrl);
    QLabel* createTextLabel(const QString& text, const QString& styleSheet = QString());
    void setupReplyView(const QString& replyToMsgId, const QString& replyToContent, 
                       const QString& replyToSender);
    
    // UI 组件
    QFrame* m_bubbleFrame = nullptr;        // 消息气泡容器
    QVBoxLayout* m_bubbleLayout = nullptr;  // 气泡布局
    QLabel* m_avatarLabel = nullptr;        // 头像
    QLabel* m_nameLabel = nullptr;          // 昵称
    QFrame* m_replyFrame = nullptr;         // 引用消息容器
    QLabel* m_replyLabel = nullptr;         // 引用消息标签
    QLabel* m_contentLabel = nullptr;       // 内容标签
    ImageLoader* m_imageLoader = nullptr;   // 图片加载器（带进度条）
    FileMessageItem* m_fileMessageItem = nullptr;  // 文件消息项
    QLabel* m_timeLabel = nullptr;          // 时间标签
    QLabel* m_statusLabel = nullptr;        // 状态标签（发送中/失败）
    QPushButton* m_retryButton = nullptr;   // 重发按钮
    
    // 数据
    QString m_msgId;
    QString m_senderName;
    bool m_isSelf = false;
    QString m_mediaUrl;
    QString m_mediaType;
    bool m_sending = false;
    bool m_sendFailed = false;
    bool m_recalled = false;
    QStringList m_mentions;  // 被@的用户 ID 列表
    
    // 引用消息相关
    QString m_replyToMsgId;      // 回复的目标消息 ID
    QString m_replyToContent;    // 回复的目标消息内容
    QString m_replyToSender;     // 回复的目标消息发送者
};
