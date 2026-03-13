#pragma once

#include <QWidget>
#include <QString>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QFutureWatcher>

class QVBoxLayout;
class QHBoxLayout;
class QStackedWidget;

/**
 * 带进度指示器的图片加载组件
 * 
 * 功能：
 * - 异步加载图片（不阻塞 UI）
 * - 显示加载进度条
 * - 显示加载状态（加载中/成功/失败）
 * - 支持取消加载
 * - 自动重试机制
 */
class ImageLoader : public QWidget {
    Q_OBJECT
    
public:
    explicit ImageLoader(QWidget *parent = nullptr);
    ~ImageLoader();
    
    /**
     * 设置最大尺寸（用于缩略图）
     */
    void setMaxSize(int width, int height);
    
    /**
     * 异步加载图片（支持 JPG/PNG/GIF/WebP）
     * @param imagePath 图片路径
     * @param showProgress 是否显示进度条（默认 true）
     * @param supportAnimation 是否支持动画（GIF/WebP，默认 true）
     */
    void loadImageAsync(const QString& imagePath, bool showProgress = true, bool supportAnimation = true);
    
    /**
     * 同步加载图片（用于小图）
     */
    void loadImageSync(const QString& imagePath);
    
    /**
     * 取消加载
     */
    void cancelLoading();
    
    /**
     * 设置缩放模式
     */
    void setAspectRatioMode(Qt::AspectRatioMode mode);
    
    /**
     * 获取当前显示的Pixmap
     */
    QPixmap currentPixmap() const;
    
    /**
     * 判断是否正在加载
     */
    bool isLoading() const;
    
signals:
    /**
     * 加载完成信号
     */
    void loadFinished(const QPixmap& pixmap);
    
    /**
     * 加载失败信号
     */
    void loadFailed(const QString& error);
    
    /**
     * 加载进度信号
     */
    void loadProgress(int progress);  // 0-100
    
private slots:
    void onLoadComplete();
    void onCancelClicked();
    void onRetryClicked();
    
protected:
    void paintEvent(QPaintEvent* event) override;
    
private:
    void setupUI();
    void showLoadingState();
    void showSuccessState();
    void showErrorState(const QString& error);
    void updateScaledPixmap();
    void startProgressAnimation();
    void stopProgressAnimation();
    void detectAndLoadAnimation(const QString& imagePath);
    
    // UI 组件
    QStackedWidget* m_stackedWidget;  // 状态切换容器
    QWidget* m_loadingWidget;         // 加载中页面
    QWidget* m_imageWidget;           // 图片显示页面
    QWidget* m_errorWidget;           // 错误页面
    
    QLabel* m_imageLabel;             // 图片标签（静态图）
    QMovie* m_movie;                  // 动画对象（GIF/WebP）
    QProgressBar* m_progressBar;      // 进度条
    QLabel* m_loadingLabel;           // "加载中..."文字
    QPushButton* m_cancelBtn;         // 取消按钮
    QLabel* m_errorLabel;             // 错误信息
    QPushButton* m_retryBtn;          // 重试按钮
    
    // 加载相关
    QString m_imagePath;
    QPixmap m_originalPixmap;
    QPixmap m_scaledPixmap;
    QSize m_maxSize = {300, 300};
    Qt::AspectRatioMode m_aspectRatioMode = Qt::KeepAspectRatio;
    bool m_supportAnimation = true;   // 是否支持动画
    bool m_isAnimatedImage = false;   // 当前是否为动画图片
    
    // 异步加载
    QFutureWatcher<QPixmap>* m_loadWatcher;
    bool m_isLoading = false;
    int m_loadProgress = 0;
    
    // 样式
    QString loadingStyle;
    QString errorStyle;
};
