#include "ScreenWidget.h"
#include <QPainter>

ScreenWidget::ScreenWidget(QWidget *parent) : QWidget(parent) {
    // Default size is 2x the native resolution of Oric Atmos (480x448)
    setMinimumSize(480, 448);
}

void ScreenWidget::updateFrame(const std::vector<uint32_t>& buffer, int width, int height) {
    if (buffer.size() != static_cast<size_t>(width * height)) return;

    // Build the QImage without deep copy (Qt points back to the Ula buffer momentarily)
    // We deep copy it locally for safety during the paintEvent.
    QImage tmpImage(reinterpret_cast<const uchar*>(buffer.data()), 
                    width, height, 
                    width * 4, QImage::Format_ARGB32);
    m_image = tmpImage.copy();
    
    // Trigger repaint asynchronously
    update();
}

void ScreenWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    // Draw black borders for any empty margins
    painter.fillRect(rect(), Qt::black);

    if (!m_image.isNull()) {
        // Pixel-Perfect rendering! Critical flag for retro emulation.
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        
        // Scale proportionally keeping 240:224 ratio
        QImage scaledImage = m_image.scaled(size(), Qt::KeepAspectRatio, Qt::FastTransformation);
        
        // Center the scaled image on the widget
        int x = (width() - scaledImage.width()) / 2;
        int y = (height() - scaledImage.height()) / 2;
        
        painter.drawImage(x, y, scaledImage);
    }
}
