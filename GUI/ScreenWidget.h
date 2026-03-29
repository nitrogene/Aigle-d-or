#ifndef SCREENWIDGET_H
#define SCREENWIDGET_H

#include <QWidget>
#include <QImage>
#include <vector>
#include <cstdint>

/**
 * @brief Qt Widget to display the Oric Atmos Frame Buffer purely.
 */
class ScreenWidget : public QWidget {
    Q_OBJECT
public:
    explicit ScreenWidget(QWidget *parent = nullptr);

    /**
     * @brief Accepts a new ARGB32 frame buffer and schedules a repaint.
     */
    void updateFrame(const std::vector<uint32_t>& buffer, int width, int height);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QImage m_image;
};

#endif // SCREENWIDGET_H
