#pragma once

#include <QWidget>
#include <QString>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>

class QVBoxLayout;
class QHBoxLayout;
class QFrame;

/**
 * 文件消息项组件
 * 
 * 功能：
 * - 按文件类型显示图标（文档/表格/PDF/压缩包等）
 * - 显示文件名和大小
 * - 下载按钮与进度条
 * - 下载状态管理（等待中/下载中/已完成/失败）
 */
class FileMessageItem : public QWidget {
    Q_OBJECT
    
public:
    explicit FileMessageItem(QWidget *parent = nullptr);
    ~FileMessageItem();
    
    /**
     * 设置文件信息
     * @param fileName 文件名
     * @param fileSize 文件大小（字节）
     * @param filePath 文件路径（本地或远程 URL）
     * @param isSelf 是否是自己发送的
     */
    void setFileInfo(const QString& fileName, qint64 fileSize, const QString& filePath, bool isSelf = false);
    
    /**
     * 设置下载状态
     */
    enum DownloadState {
        NotStarted,    // 未开始
        Downloading,   // 下载中
        Completed,     // 已完成
        Failed         // 失败
    };
    
    void setDownloadState(DownloadState state);
    void setDownloadProgress(qint64 received, qint64 total);
    
signals:
    /**
     * 下载请求信号
     */
    void downloadRequested(const QString& filePath);
    
    /**
     * 打开文件请求信号
     */
    void openFileRequested(const QString& filePath);
    
private slots:
    void onActionButtonClicked();
    
protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    
private:
    void setupUI();
    QIcon getFileIcon(const QString& fileName);
    QString formatFileSize(qint64 bytes);
    void updateActionButtonText();
    void updateUIState();
    
    // UI 组件
    QFrame* m_fileFrame;          // 文件容器
    QLabel* m_fileIconLabel;      // 文件图标
    QVBoxLayout* m_infoLayout;    // 信息布局
    QLabel* m_fileNameLabel;      // 文件名
    QLabel* m_fileSizeLabel;      // 文件大小
    QProgressBar* m_progressBar;  // 进度条
    QPushButton* m_actionButton;  // 下载/打开按钮
    
    // 文件信息
    QString m_fileName;
    QString m_filePath;
    qint64 m_fileSize = 0;
    DownloadState m_downloadState = NotStarted;
    qint64 m_downloadedBytes = 0;
    qint64 m_totalBytes = 0;
    bool m_isSelf = false;
};
