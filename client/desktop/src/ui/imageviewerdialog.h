#pragma once

#include <QDialog>
#include <QString>
#include <QMovie>

class QLabel;
class QPushButton;
class QScrollArea;

/**
 * 图片查看器对话框
 * 
 * 功能：
 * - 显示原图（支持缩放）
 * - 支持鼠标滚轮缩放
 * - 支持拖拽移动
 * - 显示文件名和尺寸信息
 */
class ImageViewerDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit ImageViewerDialog(QWidget *parent = nullptr);
    ~ImageViewerDialog();
    
    /**
     * 设置要显示的图片
     */
    void setImage(const QString& imagePath);
    
    /**
     * 设置图片信息（可选）
     */
    void setImageInfo(const QString& fileName, int width, int height);
    
private slots:
    void onZoomIn();
    void onZoomOut();
    void onResetZoom();
    void onSave();
    void onClose();
    
protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    
private:
    void setupUI();
    void updateImageTransform();
    void loadImage(const QString& imagePath);
    
    QScrollArea* m_scrollArea;
    QLabel* m_imageLabel;
    QLabel* m_infoLabel;
    QPushButton* m_zoomInBtn;
    QPushButton* m_zoomOutBtn;
    QPushButton* m_resetBtn;
    QPushButton* m_saveBtn;
    QPushButton* m_closeBtn;
    
    QPixmap m_originalPixmap;
    QMovie* m_movie;  // 支持 GIF/WebP 动画
    qreal m_scaleFactor = 1.0;
    QString m_fileName;
    bool m_isAnimated = false;  // 是否为动画图片
};
