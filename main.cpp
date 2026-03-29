#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <vector>

#include "GUI/MainWindow.h"
#include "GUI/EmulatorThread.h"
#include "Core/Hardware/MemoryBus.h"
#include "Core/CPU/Cpu6502.h"
#include "Core/Hardware/Ula.h"
#include "Core/Hardware/Via.h"
#include "Core/Hardware/AySound.h"
#include "Core/Media/FileLoader.h"
#include "Core/Media/TapLoader.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    qDebug() << "Démarrage de l'émulateur Aigle d'Or...";

    // --- Initialisation du Core (Émulateur) ---
    // --- Initialisation du Core (Émulateur) ---
    MemoryBus bus;

    // Création du Processeur (branché au bus)
    Cpu6502 cpu(&bus);
    
    // Création de la Carte Graphique ULA (branchée au bus)
    Ula ula(&bus);
    
    // Création du Contrôleur E/S (VIA 6522) et connexion
    Via via(&cpu);
    
    // Création du Processeur Sonore (AY-3-8912) et connexion au VIA
    AySound ay;
    via.attachAySound(&ay);
    bus.attachAySound(&ay);

    bus.attachVia(&via);

    // Le CPU va s'exécuter dans son coin. On démarre en pause.
    EmulatorThread emuThread(&cpu, &ula, &via);
    emuThread.setPaused(true);
    emuThread.start();
    
    // --- Initialisation de l'Interface Graphique ---
    MainWindow w(&ula, &ay);
    w.setWindowTitle("Aigle d'Or - Oric Atmos Emulator (Qt 6.11 MSVC)");
    w.resize(800, 600);
    w.show();
    
    // Remarque : Le 6502 s'exécute désormais dans son propre QThread.
    // --- Gestion du chargement dynamique ---
    QObject::connect(&w, &MainWindow::romPathSelected, [&](const QString& path) {
        emuThread.setPaused(true);
        std::vector<uint8_t> romData;
        if (FileLoader::loadBinary(path.toStdString(), romData)) {
            if (romData.size() == 16384) {
                bus.loadRom(romData.data(), romData.size());
                cpu.reset(); // On redémarre le CPU sur le vecteur de Reset de la nouvelle ROM
                qDebug() << "Nouvelle ROM chargée et CPU réinitialisé.";
            } else {
                QMessageBox::critical(&w, "Erreur ROM", 
                    QString("Taille de ROM invalide : %1 octets.\nL'Oric Atmos requiert exactement 16384 octets.").arg(romData.size()));
            }
        }
        emuThread.setPaused(false);
    });

    QObject::connect(&w, &MainWindow::tapPathSelected, [&](const QString& path) {
        emuThread.setPaused(true);
        qDebug() << "Fast Loading TAP file :" << path;
        
        if (TapLoader::fastLoad(path.toStdString(), &bus, &cpu)) {
            qDebug() << "Chargement TAP réussi.";
            QMessageBox::information(&w, "Tape Load", "Fichier TAP chargé avec succès (Fast Load).");
        } else {
            QMessageBox::warning(&w, "Tape Load", "Échec du chargement du fichier TAP.");
        }
        emuThread.setPaused(false);
    });

    int exitCode = a.exec();
    
    qDebug() << "Extinction de l'émulateur... Arrêt du CPU.";
    emuThread.requestInterruption();
    emuThread.wait();
    
    return exitCode;
}
