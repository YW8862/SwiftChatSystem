#pragma once

#include <QString>
#include <QPixmap>

/**
 * 图片工具类
 */
namespace ImageUtils {

/**
 * 加载并缩放图片
 */
QPixmap loadScaled(const QString& path, int maxWidth, int maxHeight);

/**
 * 生成圆形头像
 */
QPixmap makeCircular(const QPixmap& source);

/**
 * 生成缩略图
 */
QPixmap makeThumbnail(const QString& path, int size);

/**
 * 获取图片文件的 MIME 类型
 */
QString getMimeType(const QString& path);

}  // namespace ImageUtils
