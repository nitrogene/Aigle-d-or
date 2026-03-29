#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

class ScreenWidget;
class Ula;
class AySound;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(Ula* ula, AySound* ay, QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void romPathSelected(const QString& path);
    void tapPathSelected(const QString& path);
    void systemResetRequested();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private slots:
    void onLoadRom();
    void onLoadTap();
    void renderTick(); // VSYNC Tick (50 Hz)

private:
    ScreenWidget* m_screen;
    Ula* m_ula;
    AySound* m_ay;
    QTimer* m_timer;
    
    // Matrice de mapping (Row, Col pour chaque Qt::Key)
    struct KeyPos { int row; int col; };
    void handleKeyEvent(QKeyEvent* event, bool pressed);
};

#endif // MAINWINDOW_H
