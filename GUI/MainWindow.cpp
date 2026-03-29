#include "MainWindow.h"
#include "ScreenWidget.h"
#include "../Core/Hardware/Ula.h"
#include "../Core/Hardware/AySound.h"
#include <QKeyEvent>
#include <QMap>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>

MainWindow::MainWindow(Ula* ula, AySound* ay, QWidget *parent)
    : QMainWindow(parent), m_ula(ula), m_ay(ay)
{
    // --- Barre de Menus ---
    QMenu* fileMenu = menuBar()->addMenu(tr("&Fichier"));
    
    QAction* loadRomAct = new QAction(tr("Charger une &ROM..."), this);
    connect(loadRomAct, &QAction::triggered, this, &MainWindow::onLoadRom);
    fileMenu->addAction(loadRomAct);

    QAction* loadTapAct = new QAction(tr("Charger une Cassette (&TAP)..."), this);
    connect(loadTapAct, &QAction::triggered, this, &MainWindow::onLoadTap);
    fileMenu->addAction(loadTapAct);
    
    fileMenu->addSeparator();
    
    QAction* quitAct = new QAction(tr("&Quitter"), this);
    connect(quitAct, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(quitAct);

    // --- Widget Ecran ---
    m_screen = new ScreenWidget(this);
    setCentralWidget(m_screen);

    // Initialisation de la VSYNC (50 Hz, un rafraichissement toutes les 20 ms)
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::renderTick);
    
    // Le vrai Oric tourne sur TV PAL (50 fps)
    m_timer->start(20); 
}

MainWindow::~MainWindow() {}

void MainWindow::renderTick()
{
    if (m_ula) {
        // Demande au Core de scanner la VRAM (0xBB80 -> 0xBFDF) et de générer l'image RVB
        m_ula->renderFrame();
        
        // Extrait le Frame Buffer du GPU et le pousse dans MainWindow pour l'affichage
        m_screen->updateFrame(m_ula->getFrameBuffer(), Ula::SCREEN_WIDTH, Ula::SCREEN_HEIGHT);
    }
}

void MainWindow::onLoadRom() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Charger une ROM Oric Atmos"), "", tr("ROM Files (*.rom *.bin);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        emit romPathSelected(fileName);
    }
}

void MainWindow::onLoadTap() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Charger une Cassette Oric"), "", tr("TAP Files (*.tap);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        emit tapPathSelected(fileName);
    }
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    handleKeyEvent(event, true);
}

void MainWindow::keyReleaseEvent(QKeyEvent* event) {
    handleKeyEvent(event, false);
}

void MainWindow::handleKeyEvent(QKeyEvent* event, bool pressed) {
    if (!m_ay) return;

    // Mapping statique Qt::Key -> (Row, Col) Oric Atmos
    // Note : C'est un mapping simplifié pour le démarrage.
    static const QMap<int, KeyPos> keyMap = {
        { Qt::Key_Space,  {0, 7} },
        { Qt::Key_Left,   {0, 1} },
        { Qt::Key_Shift,  {0, 2} }, // Left Shift
        { Qt::Key_Up,     {0, 4} },
        { Qt::Key_Comma,  {0, 6} },
        
        { Qt::Key_1,      {1, 1} },
        { Qt::Key_0,      {1, 2} },
        { Qt::Key_BracketLeft,  {1, 4} },
        { Qt::Key_BracketRight, {1, 5} },
        { Qt::Key_Delete, {1, 6} },
        { Qt::Key_Backspace, {1, 6} },
        
        { Qt::Key_P,      {2, 1} },
        { Qt::Key_O,      {2, 2} },
        { Qt::Key_I,      {2, 3} },
        { Qt::Key_U,      {2, 4} },
        
        { Qt::Key_W,      {3, 1} },
        { Qt::Key_S,      {3, 2} },
        { Qt::Key_A,      {3, 3} },
        { Qt::Key_E,      {3, 6} },
        { Qt::Key_G,      {3, 7} },
        
        { Qt::Key_H,      {4, 1} },
        { Qt::Key_Y,      {4, 2} },
        
        { Qt::Key_Equal,  {5, 1} },
        { Qt::Key_Return, {5, 3} },
        { Qt::Key_Slash,  {5, 5} },
        { Qt::Key_L,      {5, 7} },
        
        { Qt::Key_8,      {6, 1} }
    };

    int key = event->key();
    if (keyMap.contains(key)) {
        KeyPos pos = keyMap.value(key);
        m_ay->updateKeyMatrix(pos.row, pos.col, pressed);
    }
}
