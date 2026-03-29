#ifndef EMULATORTHREAD_H
#define EMULATORTHREAD_H

#include <QThread>

class Cpu6502;
class Ula;
class Via;

/**
 * @brief Thread Asynchrone gérant la boucle d'exécution 1 MHz indépendamment de l'UI.
 */
class EmulatorThread : public QThread {
    Q_OBJECT
public:
    EmulatorThread(Cpu6502* cpu, Ula* ula, Via* via, QObject* parent = nullptr);

    void setPaused(bool paused) { m_paused = paused; }
    bool isPaused() const { return m_paused; }

protected:
    /**
     * @brief Boucle principale du thread (Fetch-Decode-Execute).
     */
    void run() override;

private:
    Cpu6502* m_cpu;
    Ula* m_ula;
    Via* m_via;
    bool m_paused = false;
};

#endif // EMULATORTHREAD_H
