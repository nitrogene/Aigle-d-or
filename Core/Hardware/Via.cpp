#include "Via.h"
#include "AySound.h"
#include "../CPU/Cpu6502.h"

Via::Via(Cpu6502* cpu) : m_cpu(cpu) {
    reset();
}

Via::~Via() {}

void Via::reset() {
    m_ora = 0xFF; // Généralement simulé tiré vers +5V sans clavier
    m_orb = 0xFF;
    m_ddra = 0x00;
    m_ddrb = 0x00;
    
    m_ca2 = false;
    m_cb2 = false;
    
    // Les timbers sont non nuls au boot
    m_t1C = 0xFFFF;
    m_t1L = 0xFFFF;
    m_t2C = 0xFFFF;
    
    m_acr = 0x00;
    m_pcr = 0x00;
    m_ifr = 0x00;
    m_ier = 0x00;
    checkIRQs();
}

uint8_t Via::read(uint16_t addr) {
    uint8_t reg = addr & 0x0F; // 16 registres seulement
    uint8_t val = 0;

    switch (reg) {
        case 0: { // Read Port B
            // On renvoie m_orb pour les bits en sortie, et 1 (Pull-up) pour les bits en entrée
            uint8_t val = (m_orb & m_ddrb) | (~m_ddrb & 0xFF);
            
            // Le bit 6 (Cassette Input) cause le défilement d'étoiles s'il est à 0.
            // On le force à 1 (inactif).
            val |= 0x40; 
            
            if (m_ay) {
                uint8_t row = m_orb & 0x07;
                uint8_t colMask = m_ay->getReg(14);
                if (m_ay->checkSense(row, colMask)) {
                    val &= ~0x08; // Touche pressée -> 0 (Active Low)
                } else {
                    val |= 0x08;  // Libre -> 1 (Pull-up)
                }
            }
            return val;
        }
        case 1:
            // Sur Oric : CA2=BDIR, CB2=BC1
            // Read Data = BC1=1, BDIR=0 => CB2=1, CA2=0
            if (m_cb2 && !m_ca2 && m_ay) {
                val = m_ay->readData();
            } else {
                val = m_ora;
            }
            break;
        case 2:  val = m_ddrb; break;
        case 3:  val = m_ddra; break;
        case 4: 
            val = m_t1C & 0xFF; 
            m_ifr &= ~0x40; // Acquittement de l'interruption T1
            checkIRQs();
            break;
        case 5:  val = (m_t1C >> 8) & 0xFF; break;
        case 6:  val = m_t1L & 0xFF; break;
        case 7:  val = (m_t1L >> 8) & 0xFF; break;
        case 8: 
            val = m_t2C & 0xFF; 
            m_ifr &= ~0x20; // Acquittement de l'interruption T2
            checkIRQs();
            break;
        case 9:  val = (m_t2C >> 8) & 0xFF; break;
        case 11: val = m_acr; break;
        case 12: val = m_pcr; break;
        case 13: 
            val = m_ifr; 
            if ((m_ifr & m_ier) & 0x7F) val |= 0x80; // IRQ Active globale
            else val &= ~0x80;
            break;
        case 14: val = m_ier | 0x80; break;
        case 15: val = m_ora; break;
        default: break;
    }
    return val;
}

void Via::write(uint16_t addr, uint8_t data) {
    uint8_t reg = addr & 0x0F;
    switch (reg) {
        case 0:  
            m_orb = data; 
            // Mise à jour de la ligne de clavier (PB0-PB2)
            if (m_ay) m_ay->setKeyboardRow(data & 0x07);
            break;
        case 1:  
            m_ora = data; 
            // Si AY en mode écriture ou sélection d'adresse
            updateAyControl();
            break;
        case 2:  m_ddrb = data; break;
        case 3:  m_ddra = data; break;
        case 4:  m_t1L = (m_t1L & 0xFF00) | data; break;
        case 5: 
            m_t1L = (data << 8) | (m_t1L & 0x00FF);
            m_t1C = m_t1L; // Armement du temporisateur
            m_ifr &= ~0x40; // Acquittement
            checkIRQs();
            break;
        case 6:  m_t1L = (m_t1L & 0xFF00) | data; break; 
        case 7:  m_t1L = (data << 8) | (m_t1L & 0x00FF); break;
        case 8:  m_t2C = (m_t2C & 0xFF00) | data; break;
        case 9: 
            m_t2C = (data << 8) | (m_t2C & 0x00FF);
            m_ifr &= ~0x20; // Acquittement de T2
            checkIRQs();
            break;
        case 11: m_acr = data; break;
        case 12: 
            m_pcr = data;
            // CA2 (BDIR) : bits PCR[3:1]. Mode output quand bit3=1, niveau = bit2.
            if (data & 0x08) {
                m_ca2 = (data & 0x04) != 0;
            }
            // CB2 (BC1) : bits PCR[7:5]. Mode output quand bit7=1, niveau = bit6.
            if (data & 0x80) {
                m_cb2 = (data & 0x40) != 0;
            }
            updateAyControl();
            break;
        case 13: 
            m_ifr &= ~data; // Efface les bits ciblés
            checkIRQs();
            break;
        case 14: 
            if (data & 0x80) m_ier |= (data & 0x7F);
            else m_ier &= ~(data & 0x7F);
            checkIRQs();
            break;
        case 15: m_ora = data; break;
    }
}

void Via::step(int cycles) {
    if (cycles <= 0) return;

    // --- Timer 1 ---
    int64_t nextT1 = (int64_t)m_t1C - cycles;
    while (nextT1 < 0) {
        m_ifr |= 0x40; // Levée du flag T1
        if (m_acr & 0x40) { // Mode continu (Free-run)
            nextT1 += (m_t1L + 2); // Le 6522 met 2 cycles pour recharger
        } else { // Mode One-shot
            nextT1 = -1; // Reste fêlé à -1
            break;
        }
    }
    m_t1C = (nextT1 < 0) ? 0xFFFF : (uint16_t)nextT1;
    
    // --- Timer 2 ---
    int64_t nextT2 = (int64_t)m_t2C - cycles;
    if (nextT2 < 0) {
        m_ifr |= 0x20; // Levée du flag T2
        m_t2C = 0xFFFF; // T2 ne boucle pas en mode standard
    } else {
        m_t2C = (uint16_t)nextT2;
    }

    checkIRQs();
}

void Via::checkIRQs() {
    bool irqStatus = ((m_ifr & m_ier) & 0x7F) != 0;
    if (m_cpu) m_cpu->setIrqLine(irqStatus);
}

void Via::updateAyControl() {
    if (!m_ay) return;

    // Sur Oric Atmos : CA2 = BDIR, CB2 = BC1
    //
    // PSG Operation Modes:
    // BC1=0, BDIR=0 : Inactive         (CB2=0, CA2=0)
    // BC1=0, BDIR=1 : Write Data       (CB2=0, CA2=1)
    // BC1=1, BDIR=0 : Read Data        (CB2=1, CA2=0)  -> géré dans read() case 1
    // BC1=1, BDIR=1 : Latch Address    (CB2=1, CA2=1)

    if (m_cb2 && m_ca2) {
        // BC1=1, BDIR=1 : Latch Address
        m_ay->writeAddress(m_ora);
    } else if (!m_cb2 && m_ca2) {
        // BC1=0, BDIR=1 : Write Data
        m_ay->writeData(m_ora);
    }
}
