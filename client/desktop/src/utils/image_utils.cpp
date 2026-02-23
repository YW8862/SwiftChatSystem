#include "image_utils.h"
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QFileInfo>

namespace ImageUtils {

QPixmap loadScaled(const QString& path, int maxWidth, int maxHeight) {
    QPixmap pixmap(path);
    if (pixmap.isNull()) return pixmap;
    
    return pixmap.scaled(maxWidth, maxHeight, 
                         Qt::KeepAspectRatio, 
                         Qt::SmoothTransformation);
}

QPixmap makeCircular(const QPixmap& source) {
    if (source.isNull()) return source;
    
    int size = qMin(source.width(), source.height());
    QPixmap result(size, size);
    result.fill(Qt::transparent);
    
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    QPainterPath path;
    path.addEllipse(0, 0, size, size);
    painter.setClipPath(path);
    
    painter.drawPixmap(0, 0, source.scaled(size, size, 
                       Qt::KeepAspectRatioByExpanding, 
                       Qt::SmoothTransformation));
    
    return result;
}

QPixmap makeThumbnail(const QString& path, int size) {
    return loadScaled(path, size, size);
}

QString getMimeType(const QString& path) {
    QFileInfo info(path);
    QString suffix = info.suffix().toLower();
    
    if (suffix == "jpg" || suffix == "jpeg") return "image/jpeg";
    if (suffix == "png") return "image/png";
    if (suffix == "gif") return "image/gif";
    if (suffix == "webp") return "image/webp";
    if (suffix == "bmp") return "image/bmp";
    
    return "application/octet-stream";
}

}  // namespace ImageUtils
