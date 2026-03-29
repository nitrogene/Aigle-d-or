#include "TapLoader.h"
#include "FileLoader.h"
#include "../Hardware/MemoryBus.h"
#include "../CPU/Cpu6502.h"
#include <iostream>
#include <cstring>
#include <iomanip>
#include <sstream>

bool TapLoader::fastLoad(const std::string& filepath, MemoryBus* bus, Cpu6502* cpu) {
    if (!bus || !cpu) return false;

    std::vector<uint8_t> data;
    if (!FileLoader::loadBinary(filepath, data)) return false;

    if (data.size() < 10) return false;

    // --- Parsing de l'en-tête (Oric Atmos Format) ---
    // Un fichier TAP commence par une série d'octets de synchronisation $16,
    // puis $24 $24 $24, puis l'en-tête de 9 octets.
    
    size_t offset = 0;
    // Passe les $16
    while (offset < data.size() && data[offset] == 0x16) offset++;
    
    // Passe les $24 (Sync Marker)
    // Le format standard Oric attend au moins un $24 après la synchro $16.
    if (offset < data.size() && data[offset] == 0x24) {
        while (offset < data.size() && data[offset] == 0x24) offset++;
    } else {
        std::cerr << "[TapLoader] Format TAP Oric invalide (Signature $24 manquante).\n";
        return false;
    }

    // Header 9 octets
    if (offset + 9 >= data.size()) return false;
    
    // Octet 0: Unused (00)
    // Octet 1: Unused (00)
    // Octet 2: Flag (00=Basic, 80=Mcode)
    uint8_t type = data[offset + 2];
    // Octet 3: Autorun (00=No, 01=Yes)
    uint8_t autorun = data[offset + 3];
    // Octet 4-5: End Address (High, Low)
    uint16_t endAddr = (data[offset + 4] << 8) | data[offset + 5];
    // Octet 6-7: Start Address (High, Low)
    uint16_t startAddr = (data[offset + 6] << 8) | data[offset + 7];
    // Octet 8: Unused
    offset += 9;

    // Filename: Null-terminated
    std::string filename = "";
    while (offset < data.size() && data[offset] != 0x00) {
        filename += (char)data[offset];
        offset++;
    }
    offset++; // Saute le nul

    std::cout << "[TapLoader] Fichier : " << filename << "\n";
    std::cout << "[TapLoader] Type : " << (type == 0x80 ? "Code Machine" : "BASIC") << "\n";
    std::cout << "[TapLoader] Adresse : 0x" << std::hex << std::setw(4) << std::setfill('0') << startAddr 
              << " -> 0x" << std::hex << std::setw(4) << std::setfill('0') << endAddr << std::dec << "\n";

    // --- Injection en Mémoire ---
    // Les octets restants sont les données du programme.
    size_t bytesToCopy = data.size() - offset;
    uint16_t currentAddr = startAddr;

    for (size_t i = 0; i < bytesToCopy && currentAddr <= endAddr; ++i) {
        bus->write(currentAddr, data[offset + i]);
        currentAddr++;
    }

    std::cout << "[TapLoader] Fast Load termine. " << bytesToCopy << " octets copies.\n";

    // --- Post-Chargement (Basic/Mcode) ---
    if (type == 0x00) {
        // Pour les fichiers BASIC, on met à jour les pointeurs système de l'Atmos
        // $2D-$2E : Début de la mémoire BASIC (souvent $0501)
        // $42-$43 : Fin de la mémoire BASIC
        // $44-$45 : Fin des variables
        bus->write(0x2D, startAddr & 0xFF);
        bus->write(0x2E, (startAddr >> 8) & 0xFF);
        bus->write(0x42, endAddr & 0xFF);
        bus->write(0x43, (endAddr >> 8) & 0xFF);
        bus->write(0x44, endAddr & 0xFF);
        bus->write(0x45, (endAddr >> 8) & 0xFF);
        std::cout << "[TapLoader] Pointeurs BASIC mis a jour.\n";
    }

    if (autorun) {
        if (type == 0x80) {
            cpu->setPC(startAddr);
            std::cout << "[TapLoader] Autorun: PC force a 0x" << std::hex << startAddr << std::dec << "\n";
        }
    }
    
    return true;
}
