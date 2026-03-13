#include "imagepreviewdialog.h"

#include <QFileInfo>
#include <QPixmap>
#include <QCheckBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QApplication>
#include <QScreen>

ImagePreviewDialog::ImagePreviewDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(QStringLiteral("选择图片"));
    setMinimumSize(600, 500);
    resize(700, 550);
    
    setupUI();
}

ImagePreviewDialog::~ImagePreviewDialog() = default;

void ImagePreviewDialog::setupUI() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setSpacing(0);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    
    // 顶部工具栏
    auto* headerFrame = new QFrame(this);
    headerFrame->setStyleSheet(
        "QFrame { background: #f5f5f5; border-bottom: 1px solid #e0e0e0; }"
    );
    auto* headerLayout = new QHBoxLayout(headerFrame);
    headerLayout->setContentsMargins(16, 12, 16, 12);
    headerLayout->setSpacing(12);
    
    m_selectAllBtn = new QPushButton(QStringLiteral("全选"), this);
    m_selectAllBtn->setCheckable(true);
    m_selectAllBtn->setFixedSize(80, 32);
    m_selectAllBtn->setStyleSheet(
        "QPushButton { background: #ffffff; border: 1px solid #d0d0d0; border-radius: 4px; color: #333; font-size: 13px; }"
        "QPushButton:checked { background: #07c160; border-color: #07c160; color: white; }"
        "QPushButton:hover { background: #f0f0f0; }"
    );
    headerLayout->addWidget(m_selectAllBtn);
    
    m_countLabel = new QLabel(QStringLiteral("已选择 0 张图片"), this);
    m_countLabel->setStyleSheet("color: #666; font-size: 13px;");
    headerLayout->addWidget(m_countLabel);
    
    headerLayout->addStretch();
    rootLayout->addWidget(headerFrame);
    
    // 中间滚动区域（缩略图列表）
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setStyleSheet("QScrollArea { background: #ffffff; border: none; }");
    
    m_contentWidget = new QWidget();
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setSpacing(12);
    m_contentLayout->setContentsMargins(16, 16, 16, 16);
    
    m_scrollArea->setWidget(m_contentWidget);
    rootLayout->addWidget(m_scrollArea, 1);
    
    // 底部按钮区域
    auto* footerFrame = new QFrame(this);
    footerFrame->setStyleSheet(
        "QFrame { background: #ffffff; border-top: 1px solid #e0e0e0; }"
    );
    auto* footerLayout = new QHBoxLayout(footerFrame);
    footerLayout->setContentsMargins(16, 12, 16, 12);
    footerLayout->setSpacing(12);
    footerLayout->addStretch();
    
    m_cancelBtn = new QPushButton(QStringLiteral("取消"), this);
    m_cancelBtn->setFixedSize(100, 36);
    m_cancelBtn->setStyleSheet(
        "QPushButton { background: #ffffff; border: 1px solid #d0d0d0; border-radius: 6px; color: #333; font-size: 14px; }"
        "QPushButton:hover { background: #f5f5f5; }"
    );
    footerLayout->addWidget(m_cancelBtn);
    
    m_confirmBtn = new QPushButton(QStringLiteral("发送"), this);
    m_confirmBtn->setFixedSize(100, 36);
    m_confirmBtn->setEnabled(false);
    m_confirmBtn->setStyleSheet(
        "QPushButton { background: #07c160; border: none; border-radius: 6px; color: white; font-size: 14px; font-weight: 600; }"
        "QPushButton:hover { background: #06ad56; }"
        "QPushButton:disabled { background: #9adbb9; color: #edf9f3; }"
    );
    footerLayout->addWidget(m_confirmBtn);
    
    rootLayout->addWidget(footerFrame);
    
    // 连接信号
    connect(m_selectAllBtn, &QPushButton::clicked, this, &ImagePreviewDialog::onSelectAllClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &ImagePreviewDialog::onCancelClicked);
    connect(m_confirmBtn, &QPushButton::clicked, this, &ImagePreviewDialog::onConfirmClicked);
}

void ImagePreviewDialog::setImages(const QStringList& imagePaths) {
    m_imagePaths = imagePaths;
    m_selectedImages.clear();
    m_checkBoxes.clear();
    
    // 清空现有内容
    QLayoutItem* item;
    while ((item = m_contentLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
    
    loadThumbnails();
    updateCountLabel();
}

void ImagePreviewDialog::loadThumbnails() {
    const int thumbnailSize = 120;
    
    for (int i = 0; i < m_imagePaths.size(); ++i) {
        const QString& imagePath = m_imagePaths.at(i);
        QWidget* itemWidget = createImageItem(imagePath, i);
        m_contentLayout->addWidget(itemWidget);
    }
    
    m_contentLayout->addStretch();
}

QWidget* ImagePreviewDialog::createImageItem(const QString& imagePath, int index) {
    QFileInfo fileInfo(imagePath);
    const int thumbnailSize = 120;  // 修复：添加变量定义
    
    auto* itemFrame = new QFrame();
    itemFrame->setStyleSheet(
        "QFrame { background: #fafafa; border: 2px solid transparent; border-radius: 8px; padding: 8px; }"
        "QFrame:hover { background: #f0f0f0; }"
    );
    itemFrame->setAttribute(Qt::WA_Hover);
    
    auto* layout = new QHBoxLayout(itemFrame);
    layout->setSpacing(12);
    layout->setContentsMargins(8, 8, 8, 8);
    
    // 复选框
    auto* checkBox = new QCheckBox();
    checkBox->setCursor(Qt::PointingHandCursor);
    checkBox->setChecked(true);  // 默认选中
    m_checkBoxes.push_back(checkBox);
    
    connect(checkBox, &QCheckBox::toggled, this, [this, index](bool checked) {
        onImageSelected(index, checked);
    });
    
    layout->addWidget(checkBox);
    
    // 缩略图
    auto* thumbnailLabel = new QLabel();
    thumbnailLabel->setFixedSize(thumbnailSize, thumbnailSize);
    thumbnailLabel->setStyleSheet("background: #e8e8e8; border-radius: 4px;");
    thumbnailLabel->setAlignment(Qt::AlignCenter);
    
    QPixmap pixmap(imagePath);
    if (!pixmap.isNull()) {
        QPixmap scaled = pixmap.scaled(thumbnailSize, thumbnailSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        thumbnailLabel->setPixmap(scaled);
    } else {
        thumbnailLabel->setText(QStringLiteral("无法加载"));
    }
    
    layout->addWidget(thumbnailLabel);
    
    // 文件信息
    auto* infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(4);
    
    auto* nameLabel = new QLabel(fileInfo.fileName());
    nameLabel->setWordWrap(true);
    nameLabel->setStyleSheet("color: #333; font-size: 13px; font-weight: 500;");
    infoLayout->addWidget(nameLabel);
    
    auto* sizeLabel = new QLabel(formatFileSize(fileInfo.size()));
    sizeLabel->setStyleSheet("color: #999; font-size: 12px;");
    infoLayout->addWidget(sizeLabel);
    
    infoLayout->addStretch();
    layout->addLayout(infoLayout);
    
    return itemFrame;
}

QString ImagePreviewDialog::formatFileSize(qint64 size) const {
    const QStringList units = {"B", "KB", "MB", "GB"};
    int unitIndex = 0;
    double fileSize = size;
    
    while (fileSize >= 1024 && unitIndex < units.size() - 1) {
        fileSize /= 1024;
        unitIndex++;
    }
    
    return QString("%1 %2").arg(fileSize, 0, 'f', fileSize < 10 ? 2 : 1).arg(units[unitIndex]);
}

void ImagePreviewDialog::onSelectAllClicked(bool checked) {
    for (auto* checkBox : m_checkBoxes) {
        checkBox->setChecked(checked);
    }
}

void ImagePreviewDialog::onImageSelected(int index, bool selected) {
    if (index < 0 || index >= m_imagePaths.size()) return;
    
    if (selected) {
        if (!m_selectedImages.contains(m_imagePaths[index])) {
            m_selectedImages.append(m_imagePaths[index]);
        }
    } else {
        m_selectedImages.removeAll(m_imagePaths[index]);
    }
    
    updateCountLabel();
    
    // 更新全选按钮状态
    bool allSelected = (m_selectedImages.size() == static_cast<size_t>(m_imagePaths.size()));
    m_selectAllBtn->blockSignals(true);
    m_selectAllBtn->setChecked(allSelected);
    m_selectAllBtn->blockSignals(false);
}

void ImagePreviewDialog::updateCountLabel() {
    m_countLabel->setText(QString("已选择 %1 张图片").arg(m_selectedImages.size()));
    m_confirmBtn->setEnabled(!m_selectedImages.isEmpty());
}

void ImagePreviewDialog::onConfirmClicked() {
    accept();
}

void ImagePreviewDialog::onCancelClicked() {
    reject();
}

QStringList ImagePreviewDialog::selectedImages() const {
    return m_selectedImages;
}
