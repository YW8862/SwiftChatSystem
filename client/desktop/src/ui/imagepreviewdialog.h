#pragma once

#include <QDialog>
#include <QStringList>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QCheckBox>

/**
 * 图片预览选择对话框
 * 
 * 功能：
 * - 支持多选图片文件
 * - 显示缩略图预览
 * - 显示文件名和大小
 * - 支持全选/取消全选
 */
class ImagePreviewDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit ImagePreviewDialog(QWidget *parent = nullptr);
    ~ImagePreviewDialog();
    
    /**
     * 设置要预览的图片路径列表
     */
    void setImages(const QStringList& imagePaths);
    
    /**
     * 获取选中的图片路径列表
     */
    QStringList selectedImages() const;
    
private slots:
    void onSelectAllClicked(bool checked);
    void onImageSelected(int index, bool selected);
    void onConfirmClicked();
    void onCancelClicked();
    
private:
    void setupUI();
    void loadThumbnails();
    QWidget* createImageItem(const QString& imagePath, int index);
    QString formatFileSize(qint64 size) const;
    void updateCountLabel();
    
    QStringList m_imagePaths;
    QStringList m_selectedImages;
    QScrollArea* m_scrollArea;
    QWidget* m_contentWidget;
    QVBoxLayout* m_contentLayout;
    QPushButton* m_confirmBtn;
    QPushButton* m_cancelBtn;
    QPushButton* m_selectAllBtn;
    QLabel* m_countLabel;
    std::vector<QCheckBox*> m_checkBoxes;
};
