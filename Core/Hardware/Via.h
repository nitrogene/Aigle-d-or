#ifndef VIA_H
#define VIA_H

#include <cstdint>

class Cpu6502;
class AySound;

/**
 * @brief Versatile Interface Adapter (VIA) 6522.
 * Gère le timing général (Timer 1 et 2) et les E/S globales (Clavier, Son AY).
 */
class Via {
public:
    Via(Cpu6502* cpu);
    ~Via();

    void reset();

    /**
     * @brief Connecte le processeur sonore à ce VIA.
     */
    void attachAySound(AySound* ay) { m_ay = ay; }

    // Connexions depuis le MemoryBus (Mappées en 0x0300 - 0x030F)
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t data);

    // Fonction de synchronisation parfaite avec les cycles horloge du 6502
    void step(int cycles);

private:
    Cpu6502* m_cpu;

    // Registres ORA (Port A) et ORB (Port B)
    uint8_t m_ora;
    uint8_t m_orb;
    uint8_t m_ddra; // Directions (1 = Output, 0 = Input)
    uint8_t m_ddrb;

    // Compteurs et Latches pour Timer 1 et 2
    uint16_t m_t1C; // Compteur T1
    uint16_t m_t1L; // Latch (Verrou) du T1
    uint16_t m_t2C; // Compteur T2

    uint8_t m_acr; // Auxiliary Control Register (Mode des timers)
    uint8_t m_pcr; // Peripheral Control Register
    uint8_t m_ifr; // Interrupt Flag Register (Levée des signaux)
    uint8_t m_ier; // Interrupt Enable Register (Masques authorisés)

    // Vérifie et envoie la tension sur la broche IRQ au processeur
    void checkIRQs();

    // --- Extension Oric Atmos : Connexion PSG AY-3-8912 ---
    AySound* m_ay = nullptr;
    
    // Etats des sorties de contrôle (PCR/CA2/CB2)
    bool m_ca2 = false; // BDIR
    bool m_cb2 = false; // BC1

    /**
     * @brief Détermine le mode d'opération de l'AY en fonction de CA2/CB2.
     */
    void updateAyControl();
};

#endif // VIA_H
