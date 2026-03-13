#include "filemessageitem.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QFrame>
#include <QFileInfo>
#include <QIcon>
#include <QCursor>
#include <QMouseEvent>
#include <QMimeDatabase>
#include <QMimeType>

FileMessageItem::FileMessageItem(QWidget *parent)
    : QWidget(parent) {
    setupUI();
}

FileMessageItem::~FileMessageItem() = default;

void FileMessageItem::setupUI() {
    auto* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(8);
    
    // 文件容器
    m_fileFrame = new QFrame(this);
    m_fileFrame->setStyleSheet(
        "QFrame {"
        "   background: #f5f5f5;"
        "   border: 1px solid #e0e0e0;"
        "   border-radius: 6px;"
        "}"
        "QFrame:hover {"
        "   border-color: #3498db;"
        "   background: #f0f7ff;"
        "}"
    );
    auto* fileLayout = new QHBoxLayout(m_fileFrame);
    fileLayout->setContentsMargins(12, 10, 12, 10);
    fileLayout->setSpacing(12);
    
    // 文件图标
    m_fileIconLabel = new QLabel(m_fileFrame);
    m_fileIconLabel->setFixedSize(40, 40);
    m_fileIconLabel->setAlignment(Qt::AlignCenter);
    m_fileIconLabel->setStyleSheet("font-size: 32px;");
    fileLayout->addWidget(m_fileIconLabel);
    
    // 信息区域
    auto* infoContainer = new QWidget(m_fileFrame);
    m_infoLayout = new QVBoxLayout(infoContainer);
    m_infoLayout->setContentsMargins(0, 0, 0, 0);
    m_infoLayout->setSpacing(4);
    
    // 文件名
    m_fileNameLabel = new QLabel(infoContainer);
    m_fileNameLabel->setStyleSheet("color: #1f1f1f; font-size: 14px; font-weight: 500;");
    m_fileNameLabel->setWordWrap(true);
    m_fileNameLabel->setTextInteractionFlags(Qt::NoTextInteraction);
    m_infoLayout->addWidget(m_fileNameLabel);
    
    // 文件大小和进度信息
    m_fileSizeLabel = new QLabel(infoContainer);
    m_fileSizeLabel->setStyleSheet("color: #999; font-size: 12px;");
    m_infoLayout->addWidget(m_fileSizeLabel);
    
    // 进度条（默认隐藏）
    m_progressBar = new QProgressBar(infoContainer);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setFormat("%p%");
    m_progressBar->setFixedHeight(6);
    m_progressBar->setVisible(false);
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "   background: #e0e0e0;"
        "   border: none;"
        "   border-radius: 3px;"
        "   text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "                               stop:0 #3498db, stop:1 #2ecc71);"
        "   border-radius: 3px;"
        "}"
    );
    m_infoLayout->addWidget(m_progressBar);
    
    fileLayout->addWidget(infoContainer, 1);  // stretch=1，占据剩余空间
    
    // 操作按钮
    m_actionButton = new QPushButton(m_fileFrame);
    m_actionButton->setFixedSize(70, 32);
    m_actionButton->setCursor(Qt::PointingHandCursor);
    m_actionButton->setVisible(false);  // 初始隐藏，根据状态显示
    m_actionButton->setStyleSheet(
        "QPushButton { background: #3498db; color: white; border: none; border-radius: 6px; font-size: 13px; }"
        "QPushButton:hover { background: #2980b9; }"
        "QPushButton:disabled { background: #bdc3c7; }"
    );
    connect(m_actionButton, &QPushButton::clicked, this, &FileMessageItem::onActionButtonClicked);
    fileLayout->addWidget(m_actionButton);
    
    rootLayout->addWidget(m_fileFrame);
    
    // 安装事件过滤器
    m_fileFrame->installEventFilter(this);
}

QIcon FileMessageItem::getFileIcon(const QString& fileName) {
    QFileInfo fileInfo(fileName);
    QString suffix = fileInfo.suffix().toLower();
    
    // 根据文件类型返回不同的图标
    if (suffix == "pdf") {
        return QIcon::fromTheme("application-pdf", QIcon());
    } else if (suffix == "doc" || suffix == "docx") {
        return QIcon::fromTheme("application-msword", QIcon());
    } else if (suffix == "xls" || suffix == "xlsx") {
        return QIcon::fromTheme("application-vnd.ms-excel", QIcon());
    } else if (suffix == "ppt" || suffix == "pptx") {
        return QIcon::fromTheme("application-vnd.ms-powerpoint", QIcon());
    } else if (suffix == "txt" || suffix == "log") {
        return QIcon::fromTheme("text-plain", QIcon());
    } else if (suffix == "zip" || suffix == "rar" || suffix == "7z" || suffix == "tar" || suffix == "gz") {
        return QIcon::fromTheme("application-x-archive", QIcon());
    } else if (suffix == "jpg" || suffix == "jpeg" || suffix == "png" || suffix == "gif" || suffix == "webp") {
        return QIcon::fromTheme("image-x-generic", QIcon());
    } else if (suffix == "mp3" || suffix == "wav" || suffix == "flac") {
        return QIcon::fromTheme("audio-x-generic", QIcon());
    } else if (suffix == "mp4" || suffix == "avi" || suffix == "mkv" || suffix == "mov") {
        return QIcon::fromTheme("video-x-generic", QIcon());
    } else if (suffix == "exe" || suffix == "msi") {
        return QIcon::fromTheme("application-x-executable", QIcon());
    }
    
    // 默认文件图标
    return QIcon::fromTheme("text-x-generic", QIcon());
}

QString FileMessageItem::formatFileSize(qint64 bytes) {
    if (bytes < 0) return "0 B";
    
    const QStringList units = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unitIndex < units.size() - 1) {
        size /= 1024.0;
        unitIndex++;
    }
    
    return QString("%1 %2").arg(size, 0, 'f', unitIndex == 0 ? 0 : 1).arg(units[unitIndex]);
}

void FileMessageItem::setFileInfo(const QString& fileName, qint64 fileSize, const QString& filePath, bool isSelf) {
    m_fileName = fileName;
    m_fileSize = fileSize;
    m_filePath = filePath;
    m_isSelf = isSelf;
    
    // 设置文件图标
    QIcon icon = getFileIcon(fileName);
    if (!icon.isNull()) {
        m_fileIconLabel->setPixmap(icon.pixmap(40, 40));
    } else {
        // 如果没有图标，使用 emoji
        m_fileIconLabel->setText(QStringLiteral("📄"));
    }
    
    // 设置文件名
    m_fileNameLabel->setText(fileName);
    
    // 设置文件大小
    m_fileSizeLabel->setText(formatFileSize(fileSize));
    
    // 更新按钮文本
    updateActionButtonText();
    
    // 更新 UI 状态
    updateUIState();
}

void FileMessageItem::setDownloadState(DownloadState state) {
    m_downloadState = state;
    updateActionButtonText();
    updateUIState();
}

void FileMessageItem::setDownloadProgress(qint64 received, qint64 total) {
    m_downloadedBytes = received;
    m_totalBytes = total;
    
    if (total > 0) {
        int percent = static_cast<int>((received * 100) / total);
        m_progressBar->setValue(percent);
        m_fileSizeLabel->setText(QString("%1 / %2 (%3%)")
            .arg(formatFileSize(received))
            .arg(formatFileSize(total))
            .arg(percent));
    }
}

void FileMessageItem::updateActionButtonText() {
    switch (m_downloadState) {
        case NotStarted:
            m_actionButton->setText(QStringLiteral("⬇ 下载"));
            break;
        case Downloading:
            m_actionButton->setText(QStringLiteral("⏳ 下载中"));
            break;
        case Completed:
            m_actionButton->setText(QStringLiteral("📂 打开"));
            break;
        case Failed:
            m_actionButton->setText(QStringLiteral("↻ 重试"));
            break;
    }
}

void FileMessageItem::updateUIState() {
    switch (m_downloadState) {
        case NotStarted:
            m_progressBar->hide();
            m_fileSizeLabel->show();
            m_actionButton->show();
            m_actionButton->setEnabled(true);
            break;
            
        case Downloading:
            m_progressBar->show();
            m_fileSizeLabel->show();
            m_actionButton->hide();
            break;
            
        case Completed:
            m_progressBar->hide();
            m_fileSizeLabel->show();
            m_actionButton->show();
            m_actionButton->setEnabled(true);
            break;
            
        case Failed:
            m_progressBar->hide();
            m_fileSizeLabel->setText(m_fileSizeLabel->text() + QStringLiteral(" (下载失败)"));
            m_actionButton->show();
            m_actionButton->setEnabled(true);
            break;
    }
}

void FileMessageItem::onActionButtonClicked() {
    switch (m_downloadState) {
        case NotStarted:
        case Failed:
            emit downloadRequested(m_filePath);
            break;
            
        case Completed:
            emit openFileRequested(m_filePath);
            break;
            
        case Downloading:
            // 下载中不响应点击
            break;
    }
}

bool FileMessageItem::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_fileFrame && event->type() == QEvent::MouseButtonDblClick) {
        // 双击文件框，触发操作
        onActionButtonClicked();
        return true;
    }
    return QWidget::eventFilter(obj, event);
}
