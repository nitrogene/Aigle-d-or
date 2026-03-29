#include "AySound.h"

AySound::AySound() {
    reset();
}

AySound::~AySound() {}

void AySound::reset() {
    m_selectedReg = 0;
    for (int i = 0; i < 16; ++i) m_regs[i] = 0;
    
    // Le clavier par défaut est 'libre' (0xFF sur le port 14)
    m_regs[14] = 0xFF; 
    
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            m_keys[r][c] = false;
        }
    }
    m_activeRowIdx = 0;
}

void AySound::writeAddress(uint8_t addr) {
    if (addr < 16) m_selectedReg = addr;
}

void AySound::writeData(uint8_t data) {
    if (m_selectedReg < 16) {
        m_regs[m_selectedReg] = data;
    }
}

uint8_t AySound::readData() {
    if (m_selectedReg == 14) {
        // Lecture de la matrice clavier
        updateKeyboardPort();
    }
    return m_regs[m_selectedReg];
}

void AySound::setKeyboardRow(uint8_t rowIndex) {
    m_activeRowIdx = rowIndex & 0x07;
}

void AySound::updateKeyMatrix(int row, int col, bool pressed) {
    if (row >= 0 && row < 8 && col >= 0 && col < 8) {
        m_keys[row][col] = pressed;
    }
}

void AySound::updateKeyboardPort() {
    // Cette fonction reste pour une éventuelle compatibilité Oric-1 (lecture directe du Port A)
    // Mais elle NE DOIT PLUS mettre à jour m_regs[14] automatiquement !
    
    // Le registre 14 sur Oric Atmos est principalement conduit par le CPU
    // pour sélectionner les colonnes (Masque inverse).
}

bool AySound::checkSense(uint8_t row, uint8_t colMask) const {
    if (row >= 8 || colMask == 0xFF) return false;
    
    // Pour chaque colonne, si elle est selectionnée (bit à 0 dans le masque)
    for (int col = 0; col < 8; ++col) {
        bool colSelected = (colMask & (1 << col)) == 0;
        if (colSelected && m_keys[row][col]) {
            return true; // Connexion établie !
        }
    }
    return false;
}
