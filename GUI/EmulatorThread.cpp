#include "EmulatorThread.h"
#include "../Core/CPU/Cpu6502.h"
#include "../Core/Hardware/Ula.h"
#include "../Core/Hardware/Via.h"
#include <QDebug>

EmulatorThread::EmulatorThread(Cpu6502* cpu, Ula* ula, Via* via, QObject* parent)
    : QThread(parent), m_cpu(cpu), m_ula(ula), m_via(via) {}

void EmulatorThread::run() {
    qDebug() << "[EmulatorThread] Horloge démarrée à 1 MHz...";

    // Vitesse de l'Oric 1 MHz : ~20 000 instructions horloge par milliseconde
    // Pour se caler sur l'affichage TV (50 FPS = 20ms par frame) = ~20 000 cycles par Frame.
    const int CYCLES_PER_FRAME = 20000;

    while (!isInterruptionRequested()) {
        if (m_paused) {
            msleep(20);
            continue;
        }

        if (m_cpu->isHalted()) {
            qWarning() << "[EmulatorThread] Le CPU 6502 est bloqué (Halted). Arrêt de la boucle.";
            break;
        }

        int cyclesThisFrame = 0;
        
        // Rafale d'exécution pour couvrir 20ms temps réel
        while (cyclesThisFrame < CYCLES_PER_FRAME && !m_cpu->isHalted()) {
            int consumedCycles = m_cpu->step();
            if (m_via) m_via->step(consumedCycles);
            cyclesThisFrame += consumedCycles;
        }

        // Sommeil asynchrone pour ne pas emballer un thread à 100% CPU
        // NB: Très basique, une vraie synchronisation audio/vsync sera nécessaire plus tard.
        msleep(20);
    }
}
