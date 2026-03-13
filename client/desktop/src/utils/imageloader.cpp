#include "imageloader.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QStackedWidget>
#include <QPixmap>
#include <QFutureWatcher>
#include <QTimer>
#include <QPainter>
#include <QPaintEvent>
#include <QMovie>
#include <QFileInfo>

ImageLoader::ImageLoader(QWidget *parent)
    : QWidget(parent) {
    m_movie = nullptr;
    setupUI();
}

ImageLoader::~ImageLoader() {
    cancelLoading();
    if (m_movie) {
        m_movie->stop();
        delete m_movie;
        m_movie = nullptr;
    }
}

void ImageLoader::setupUI() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    
    // 创建堆叠容器用于状态切换
    m_stackedWidget = new QStackedWidget(this);
    
    // === 加载中页面 ===
    m_loadingWidget = new QWidget();
    auto* loadingLayout = new QVBoxLayout(m_loadingWidget);
    loadingLayout->setSpacing(16);
    loadingLayout->setAlignment(Qt::AlignCenter);
    
    // 加载动画（可选，使用 QMovie 显示旋转图标）
    m_loadingLabel = new QLabel(QStringLiteral("🔄"));
    m_loadingLabel->setAlignment(Qt::AlignCenter);
    m_loadingLabel->setStyleSheet("font-size: 32px; color: #999;");
    loadingLayout->addWidget(m_loadingLabel);
    
    // 进度条
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(6);
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
    loadingLayout->addWidget(m_progressBar);
    
    // 加载提示文字
    auto* progressText = new QLabel(QStringLiteral("正在加载图片..."));
    progressText->setAlignment(Qt::AlignCenter);
    progressText->setStyleSheet("color: #999; font-size: 12px;");
    loadingLayout->addWidget(progressText);
    
    // 取消按钮
    m_cancelBtn = new QPushButton(QStringLiteral("取消"));
    m_cancelBtn->setFixedSize(80, 32);
    m_cancelBtn->setCursor(Qt::PointingHandCursor);
    m_cancelBtn->setStyleSheet(
        "QPushButton { background: #95a5a6; color: white; border: none; border-radius: 6px; font-size: 13px; }"
        "QPushButton:hover { background: #7f8c8d; }"
    );
    connect(m_cancelBtn, &QPushButton::clicked, this, &ImageLoader::onCancelClicked);
    loadingLayout->addWidget(m_cancelBtn, 0, Qt::AlignCenter);
    
    m_stackedWidget->addWidget(m_loadingWidget);
    
    // === 图片显示页面 ===
    m_imageWidget = new QWidget();
    auto* imageLayout = new QVBoxLayout(m_imageWidget);
    imageLayout->setContentsMargins(0, 0, 0, 0);
    imageLayout->setAlignment(Qt::AlignCenter);
    
    m_imageLabel = new QLabel();
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setBackgroundRole(QPalette::Light);
    m_imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLayout->addWidget(m_imageLabel);
    
    m_stackedWidget->addWidget(m_imageWidget);
    
    // === 错误页面 ===
    m_errorWidget = new QWidget();
    auto* errorLayout = new QVBoxLayout(m_errorWidget);
    errorLayout->setSpacing(12);
    errorLayout->setAlignment(Qt::AlignCenter);
    
    m_errorLabel = new QLabel(QStringLiteral("⚠ 图片加载失败"));
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setStyleSheet("color: #e74c3c; font-size: 14px; font-weight: 600;");
    errorLayout->addWidget(m_errorLabel);
    
    auto* errorHint = new QLabel(QStringLiteral("可能原因：文件不存在或格式不支持"));
    errorHint->setAlignment(Qt::AlignCenter);
    errorHint->setStyleSheet("color: #999; font-size: 11px;");
    errorLayout->addWidget(errorHint);
    
    // 重试按钮
    m_retryBtn = new QPushButton(QStringLiteral("↻ 重试"));
    m_retryBtn->setFixedSize(80, 32);
    m_retryBtn->setCursor(Qt::PointingHandCursor);
    m_retryBtn->setStyleSheet(
        "QPushButton { background: #3498db; color: white; border: none; border-radius: 6px; font-size: 13px; }"
        "QPushButton:hover { background: #2980b9; }"
    );
    connect(m_retryBtn, &QPushButton::clicked, this, &ImageLoader::onRetryClicked);
    errorLayout->addWidget(m_retryBtn, 0, Qt::AlignCenter);
    
    m_stackedWidget->addWidget(m_errorWidget);
    
    rootLayout->addWidget(m_stackedWidget);
    
    // 初始化异步加载监视器
    m_loadWatcher = new QFutureWatcher<QPixmap>(this);
    connect(m_loadWatcher, &QFutureWatcher<QPixmap>::finished,
            this, &ImageLoader::onLoadComplete);
    
    // 默认显示加载页面
    showLoadingState();
}

void ImageLoader::setMaxSize(int width, int height) {
    m_maxSize = QSize(width, height);
    updateScaledPixmap();  // 重新计算缩放
}

void ImageLoader::setAspectRatioMode(Qt::AspectRatioMode mode) {
    m_aspectRatioMode = mode;
    updateScaledPixmap();
}

void ImageLoader::loadImageAsync(const QString& imagePath, bool showProgress, bool supportAnimation) {
    if (m_isLoading) {
        cancelLoading();
    }
    
    m_imagePath = imagePath;
    m_supportAnimation = supportAnimation;
    m_isLoading = true;
    m_loadProgress = 0;
    
    // 显示加载状态
    if (showProgress) {
        showLoadingState();
        startProgressAnimation();
    }
    
    // 异步加载图片（使用 QTimer 模拟，避免依赖 QtConcurrent）
    m_isLoading = true;
    
    // 使用 QTimer 延迟加载，模拟异步效果
    auto* timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(10);  // 10ms 后开始加载
    
    connect(timer, &QTimer::timeout, this, [this, imagePath]() {
        sender()->deleteLater();  // 清理 timer
        
        // 检测是否为动画图片并加载
        detectAndLoadAnimation(imagePath);
    });
    
    timer->start();
}

void ImageLoader::loadImageSync(const QString& imagePath) {
    m_imagePath = imagePath;
    m_originalPixmap = QPixmap(imagePath);
    
    if (m_originalPixmap.isNull()) {
        showErrorState(QStringLiteral("无法加载图片"));
        emit loadFailed(QStringLiteral("图片加载失败"));
        return;
    }
    
    updateScaledPixmap();
    showSuccessState();
    emit loadFinished(m_scaledPixmap);
}

void ImageLoader::cancelLoading() {
    if (m_loadWatcher && m_loadWatcher->isRunning()) {
        m_loadWatcher->cancel();
    }
    
    // 停止动画
    if (m_movie) {
        m_movie->stop();
    }
    
    m_isLoading = false;
    stopProgressAnimation();
}

QPixmap ImageLoader::currentPixmap() const {
    return m_scaledPixmap;
}

bool ImageLoader::isLoading() const {
    return m_isLoading;
}

void ImageLoader::onLoadComplete() {
    m_isLoading = false;
    stopProgressAnimation();
    
    if (m_loadWatcher->isCanceled()) {
        // 用户取消加载
        return;
    }
    
    if (m_loadWatcher->isFinished()) {
        m_originalPixmap = m_loadWatcher->result();
        
        if (m_originalPixmap.isNull()) {
            showErrorState(QStringLiteral("图片加载失败"));
            emit loadFailed(QStringLiteral("无法加载图片：%1").arg(m_imagePath));
        } else {
            updateScaledPixmap();
            showSuccessState();
            emit loadFinished(m_scaledPixmap);
        }
    }
}

/**
 * 检测并加载动画图片（GIF/WebP）
 */
void ImageLoader::detectAndLoadAnimation(const QString& imagePath) {
    QFileInfo fileInfo(imagePath);
    QString suffix = fileInfo.suffix().toLower();
    
    // 判断是否为动画格式
    bool isAnimated = (suffix == "gif" || suffix == "webp") && m_supportAnimation;
    
    if (isAnimated) {
        // 尝试加载为动画
        m_movie = new QMovie(this);
        m_movie->setFileName(imagePath);
        
        if (m_movie->isValid() && m_movie->frameCount() > 1) {
            // 确认为动画
            m_isAnimatedImage = true;
            
            // 设置动画到标签
            m_imageLabel->setMovie(m_movie);
            m_imageLabel->show();
            
            // 启动动画
            m_movie->start();
            
            // 隐藏进度条，显示动画
            stopProgressAnimation();
            showSuccessState();
            
            emit loadFinished(QPixmap());  // 动画不返回 pixmap
            return;
        } else {
            // 不是有效动画，回退到静态图
            delete m_movie;
            m_movie = nullptr;
            m_isAnimatedImage = false;
        }
    }
    
    // 加载静态图
    m_isAnimatedImage = false;
    m_originalPixmap = QPixmap(imagePath);
    
    onLoadComplete();
}

void ImageLoader::onCancelClicked() {
    cancelLoading();
    // 取消后显示空白或保持当前状态
    m_stackedWidget->setCurrentWidget(m_imageWidget);
}

void ImageLoader::onRetryClicked() {
    if (!m_imagePath.isEmpty()) {
        loadImageAsync(m_imagePath, true);
    }
}

void ImageLoader::showLoadingState() {
    m_stackedWidget->setCurrentWidget(m_loadingWidget);
}

void ImageLoader::showSuccessState() {
    m_stackedWidget->setCurrentWidget(m_imageWidget);
}

void ImageLoader::showErrorState(const QString& error) {
    if (m_errorLabel) {
        m_errorLabel->setText(QString("⚠ %1").arg(error));
    }
    m_stackedWidget->setCurrentWidget(m_errorWidget);
}

void ImageLoader::updateScaledPixmap() {
    if (m_originalPixmap.isNull()) return;
    
    // 如果是动画，不需要缩放（QMovie 会自动处理）
    if (m_isAnimatedImage && m_movie) {
        return;
    }
    
    // 缩放到最大尺寸
    m_scaledPixmap = m_originalPixmap.scaled(
        m_maxSize,
        m_aspectRatioMode,
        Qt::SmoothTransformation
    );
    
    if (m_imageLabel) {
        m_imageLabel->setPixmap(m_scaledPixmap);
    }
}

void ImageLoader::startProgressAnimation() {
    // 模拟进度更新（实际项目中可以根据文件大小和已读取字节数更新）
    m_loadProgress = 0;
    
    // 使用定时器模拟进度更新
    auto* timer = new QTimer(this);
    timer->setInterval(100);  // 每 100ms 更新一次
    
    connect(timer, &QTimer::timeout, this, [this, timer]() {
        if (!m_isLoading) {
            timer->stop();
            timer->deleteLater();
            return;
        }
        
        // 模拟进度增长（先快后慢）
        int increment = (100 - m_loadProgress) / 10;
        if (increment < 1) increment = 1;
        m_loadProgress += increment;
        
        if (m_loadProgress >= 90 && m_loadWatcher->isRunning()) {
            m_loadProgress = 90;  // 保持在 90% 等待完成
        }
        
        if (m_progressBar) {
            m_progressBar->setValue(m_loadProgress);
        }
        
        emit loadProgress(m_loadProgress);
        
        if (m_loadProgress >= 100 || !m_isLoading) {
            timer->stop();
            timer->deleteLater();
        }
    });
    
    timer->start();
}

void ImageLoader::stopProgressAnimation() {
    m_loadProgress = 100;
    if (m_progressBar) {
        m_progressBar->setValue(100);
    }
}

void ImageLoader::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
    // 可以在这里绘制自定义背景或边框
}
