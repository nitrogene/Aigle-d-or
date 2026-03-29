#ifndef AYSOUND_H
#define AYSOUND_H

#include <cstdint>
#include <vector>

/**
 * @brief Contrôleur Sonore PSG (Programmable Sound Generator) AY-3-8912.
 * Sur l'Oric, il gère non seulement l'audio mais aussi la Matrice du Clavier (Port A).
 */
class AySound {
public:
    AySound();
    ~AySound();

    void reset();

    // Commandes reçues du VIA 6522
    void writeAddress(uint8_t addr);
    void writeData(uint8_t data);
    uint8_t readData();

    // Signal de changement de ligne de clavier par le VIA Port B
    void setKeyboardRow(uint8_t rowIndex);

    // Lecture d'un registre
    uint8_t getReg(uint8_t reg) const { return (reg < 16) ? m_regs[reg] : 0; }

    /**
     * @brief Vérifie si une touche est pressée pour une ligne donnée et un masque de colonnes.
     * Utilisé par le VIA pour le bit PB3 (Sense).
     */
    bool checkSense(uint8_t row, uint8_t colMask) const;

    // Evénements venant de l'UI Qt Host
    // pressed = true signifie que le joueur appuie sur la touche.
    void updateKeyMatrix(int row, int col, bool pressed);

private:
    uint8_t m_selectedReg;
    uint8_t m_regs[16];

    bool m_keys[8][8];
    uint8_t m_activeRowIdx; 

    // Evalue le Port A (Reg 14) en fonction des touches pressées sur les lignes actives
    void updateKeyboardPort();
};

#endif // AYSOUND_H
