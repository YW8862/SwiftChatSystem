#include "imageviewerdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QPixmap>
#include <QFileDialog>
#include <QMessageBox>
#include <QWheelEvent>
#include <QScreen>
#include <QApplication>
#include <QFileInfo>

ImageViewerDialog::ImageViewerDialog(QWidget *parent)
    : QDialog(parent) {
    m_movie = nullptr;
    m_isAnimated = false;
    setWindowTitle(QStringLiteral("图片查看器"));
    setMinimumSize(800, 600);
    resize(900, 650);
    
    setupUI();
}

ImageViewerDialog::~ImageViewerDialog() {
    if (m_movie) {
        delete m_movie;
        m_movie = nullptr;
    }
}

void ImageViewerDialog::setupUI() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    
    // 顶部工具栏
    auto* toolbarFrame = new QFrame(this);
    toolbarFrame->setStyleSheet(
        "QFrame { background: #2b2b2b; border-bottom: 1px solid #444; }"
    );
    auto* toolbarLayout = new QHBoxLayout(toolbarFrame);
    toolbarLayout->setContentsMargins(16, 8, 16, 8);
    toolbarLayout->setSpacing(12);
    
    m_zoomInBtn = new QPushButton(QStringLiteral("🔍+"), this);
    m_zoomInBtn->setToolTip(QStringLiteral("放大 (Ctrl++)"));
    m_zoomInBtn->setFixedSize(36, 36);
    m_zoomInBtn->setStyleSheet(
        "QPushButton { background: #3c3c3c; border: none; border-radius: 6px; color: white; font-size: 16px; }"
        "QPushButton:hover { background: #4c4c4c; }"
    );
    connect(m_zoomInBtn, &QPushButton::clicked, this, &ImageViewerDialog::onZoomIn);
    toolbarLayout->addWidget(m_zoomInBtn);
    
    m_zoomOutBtn = new QPushButton(QStringLiteral("🔍-"), this);
    m_zoomOutBtn->setToolTip(QStringLiteral("缩小 (Ctrl+-)"));
    m_zoomOutBtn->setFixedSize(36, 36);
    m_zoomOutBtn->setStyleSheet(
        "QPushButton { background: #3c3c3c; border: none; border-radius: 6px; color: white; font-size: 16px; }"
        "QPushButton:hover { background: #4c4c4c; }"
    );
    connect(m_zoomOutBtn, &QPushButton::clicked, this, &ImageViewerDialog::onZoomOut);
    toolbarLayout->addWidget(m_zoomOutBtn);
    
    m_resetBtn = new QPushButton(QStringLiteral("⟲ 重置"), this);
    m_resetBtn->setToolTip(QStringLiteral("重置缩放 (Ctrl+0)"));
    m_resetBtn->setFixedSize(70, 36);
    m_resetBtn->setStyleSheet(
        "QPushButton { background: #3c3c3c; border: none; border-radius: 6px; color: white; font-size: 13px; }"
        "QPushButton:hover { background: #4c4c4c; }"
    );
    connect(m_resetBtn, &QPushButton::clicked, this, &ImageViewerDialog::onResetZoom);
    toolbarLayout->addWidget(m_resetBtn);
    
    toolbarLayout->addStretch();
    
    m_infoLabel = new QLabel(this);
    m_infoLabel->setStyleSheet("color: #ccc; font-size: 13px;");
    m_infoLabel->setTextInteractionFlags(Qt::NoTextInteraction);
    toolbarLayout->addWidget(m_infoLabel);
    
    toolbarLayout->addStretch();
    
    m_saveBtn = new QPushButton(QStringLiteral("💾 保存"), this);
    m_saveBtn->setToolTip(QStringLiteral("保存图片 (Ctrl+S)"));
    m_saveBtn->setFixedSize(80, 36);
    m_saveBtn->setStyleSheet(
        "QPushButton { background: #07c160; border: none; border-radius: 6px; color: white; font-size: 13px; font-weight: 600; }"
        "QPushButton:hover { background: #06ad56; }"
    );
    connect(m_saveBtn, &QPushButton::clicked, this, &ImageViewerDialog::onSave);
    toolbarLayout->addWidget(m_saveBtn);
    
    m_closeBtn = new QPushButton(QStringLiteral("✕ 关闭"), this);
    m_closeBtn->setToolTip(QStringLiteral("关闭 (Esc)"));
    m_closeBtn->setFixedSize(80, 36);
    m_closeBtn->setStyleSheet(
        "QPushButton { background: #c42b1c; border: none; border-radius: 6px; color: white; font-size: 13px; font-weight: 600; }"
        "QPushButton:hover { background: #d43f2c; }"
    );
    connect(m_closeBtn, &QPushButton::clicked, this, &ImageViewerDialog::onClose);
    toolbarLayout->addWidget(m_closeBtn);
    
    rootLayout->addWidget(toolbarFrame);
    
    // 中间滚动区域（显示图片）
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setAlignment(Qt::AlignCenter);
    m_scrollArea->setStyleSheet("QScrollArea { background: #1e1e1e; border: none; }");
    
    m_imageLabel = new QLabel();
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setBackgroundRole(QPalette::Dark);
    m_imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_imageLabel->setScaledContents(false);
    
    // 安装事件过滤器以支持滚轮缩放
    m_imageLabel->installEventFilter(this);
    
    m_scrollArea->setWidget(m_imageLabel);
    rootLayout->addWidget(m_scrollArea, 1);
    
    // 添加键盘快捷键
    m_zoomInBtn->setShortcut(QKeySequence::ZoomIn);
    m_zoomOutBtn->setShortcut(QKeySequence::ZoomOut);
    m_resetBtn->setShortcut(QKeySequence("Ctrl+0"));
    m_saveBtn->setShortcut(QKeySequence::Save);
}

void ImageViewerDialog::setImage(const QString& imagePath) {
    loadImage(imagePath);
}

void ImageViewerDialog::setImageInfo(const QString& fileName, int width, int height) {
    m_fileName = fileName;
    if (m_infoLabel) {
        m_infoLabel->setText(QString("%1 - %2×%3").arg(fileName).arg(width).arg(height));
    }
}

void ImageViewerDialog::loadImage(const QString& imagePath) {
    // 清理之前的动画
    if (m_movie) {
        delete m_movie;
        m_movie = nullptr;
    }
    m_isAnimated = false;
    
    // 检测是否为 GIF/WebP 动画
    QFileInfo fileInfo(imagePath);
    QString suffix = fileInfo.suffix().toLower();
    bool isAnimatedFormat = (suffix == "gif" || suffix == "webp");
    
    if (isAnimatedFormat) {
        // 尝试加载为动画
        m_movie = new QMovie(imagePath);
        
        if (m_movie->isValid() && m_movie->frameCount() > 1) {
            // 确认为动画
            m_isAnimated = true;
            
            // 设置动画到标签
            m_imageLabel->setMovie(m_movie);
            m_imageLabel->show();
            
            // 启动动画
            m_movie->start();
            
            // 更新信息标签
            if (m_fileName.isEmpty()) {
                m_fileName = fileInfo.fileName();
            }
            if (m_infoLabel) {
                m_infoLabel->setText(QString("%1 - %2×%3 (GIF 动图)")
                    .arg(m_fileName)
                    .arg(m_movie->currentPixmap().width())
                    .arg(m_movie->currentPixmap().height()));
            }
            
            m_scaleFactor = 1.0;
            return;  // 动画加载完成，直接返回
        } else {
            // 不是有效动画，回退到静态图
            delete m_movie;
            m_movie = nullptr;
        }
    }
    
    // 加载静态图
    m_originalPixmap = QPixmap(imagePath);
    
    if (m_originalPixmap.isNull()) {
        QMessageBox::warning(this, QStringLiteral("加载失败"),
                           QStringLiteral("无法加载图片：%1").arg(imagePath));
        return;
    }
    
    // 初始缩放：适应窗口大小
    QSize screenSize = QApplication::primaryScreen()->availableSize();
    int maxWidth = screenSize.width() * 0.8;
    int maxHeight = screenSize.height() * 0.8;
    
    QPixmap scaled = m_originalPixmap.scaled(maxWidth, maxHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_imageLabel->setPixmap(scaled);
    
    // 更新信息标签
    if (m_fileName.isEmpty()) {
        m_fileName = fileInfo.fileName();
    }
    if (m_infoLabel) {
        m_infoLabel->setText(QString("%1 - %2×%3")
            .arg(m_fileName)
            .arg(m_originalPixmap.width())
            .arg(m_originalPixmap.height()));
    }
    
    m_scaleFactor = 1.0;
}

void ImageViewerDialog::updateImageTransform() {
    // 如果是动画，不需要缩放（QMovie 会自动处理）
    if (m_isAnimated && m_movie) {
        return;
    }
    
    if (m_originalPixmap.isNull()) return;
    
    int newWidth = static_cast<int>(m_originalPixmap.width() * m_scaleFactor);
    int newHeight = static_cast<int>(m_originalPixmap.height() * m_scaleFactor);
    
    // 限制最小和最大缩放比例
    if (newWidth < 50 || newHeight < 50) {
        m_scaleFactor = qMax(50.0 / m_originalPixmap.width(), 50.0 / m_originalPixmap.height());
        newWidth = static_cast<int>(m_originalPixmap.width() * m_scaleFactor);
        newHeight = static_cast<int>(m_originalPixmap.height() * m_scaleFactor);
    }
    
    QPixmap scaled = m_originalPixmap.scaled(newWidth, newHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_imageLabel->setPixmap(scaled);
}

void ImageViewerDialog::onZoomIn() {
    m_scaleFactor *= 1.2;
    updateImageTransform();
}

void ImageViewerDialog::onZoomOut() {
    m_scaleFactor /= 1.2;
    updateImageTransform();
}

void ImageViewerDialog::onResetZoom() {
    m_scaleFactor = 1.0;
    updateImageTransform();
}

void ImageViewerDialog::onSave() {
    if (m_originalPixmap.isNull()) return;
    
    QString defaultName = m_fileName.isEmpty() ? "image.png" : m_fileName;
    QString filePath = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("保存图片"),
        defaultName,
        QStringLiteral("PNG 文件 (*.png);;JPEG 文件 (*.jpg *.jpeg);;所有文件 (*)")
    );
    
    if (filePath.isEmpty()) return;
    
    if (m_originalPixmap.save(filePath)) {
        QMessageBox::information(this, QStringLiteral("保存成功"),
                               QStringLiteral("图片已保存到：%1").arg(filePath));
    } else {
        QMessageBox::warning(this, QStringLiteral("保存失败"),
                           QStringLiteral("无法保存图片，请检查路径和权限"));
    }
}

void ImageViewerDialog::onClose() {
    reject();
}

// 事件过滤器：支持滚轮缩放
bool ImageViewerDialog::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_imageLabel && event->type() == QEvent::Wheel) {
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
        
        if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
            // Ctrl + 滚轮：缩放
            if (wheelEvent->angleDelta().y() > 0) {
                onZoomIn();
            } else {
                onZoomOut();
            }
            return true;
        }
    }
    
    return QDialog::eventFilter(obj, event);
}
