#include "MemoryBus.h"
#include "Via.h"
#include <cstring>
#include <iostream>

MemoryBus::MemoryBus() {
    // Allocation de tout l'espace mémoire Oric
    m_ram.resize(0x10000, 0x00);
    // ROM 16ko vide a priori
    m_rom.resize(0x4000, 0x00);
}

MemoryBus::~MemoryBus() {
}

uint8_t MemoryBus::read(uint16_t addr) {
    if (addr >= 0xC000) {
        // L'Oric Atmos mappe la ROM système dans les 16 derniers Ko
        return m_rom[addr - 0xC000];
    } else if (addr >= 0x0300 && addr <= 0x03FF) {
        // Redirection vers le registre matériel VIA 6522
        if (m_via) return m_via->read(addr);
        return 0x00;
    } else {
        // Tout le reste est de la RAM
        return m_ram[addr];
    }
}

void MemoryBus::write(uint16_t addr, uint8_t data) {
    if (addr >= 0xC000) {
        // Ne rien faire, on ne peut pas écraser la ROM
        std::cerr << "Warning: Tentative d'écriture en ROM à l'adresse 0x" << std::hex << addr << "\n";
        return;
    } else if (addr >= 0x0300 && addr <= 0x03FF) {
        // Interception par le composant matériel VIA 6522
        if (m_via) m_via->write(addr, data);
        return;
    } else {
        // On écrit la RAM normalement pour les adresses basses
        m_ram[addr] = data;
    }
}

bool MemoryBus::loadRom(const uint8_t* romData, size_t size) {
    if (size > m_rom.size() || !romData) {
        return false;
    }
    std::memcpy(m_rom.data(), romData, size);
    return true;
}
