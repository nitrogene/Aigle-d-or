#ifndef TAPLOADER_H
#define TAPLOADER_H

#include <string>
#include <vector>
#include <cstdint>

class MemoryBus;
class Cpu6502;

/**
 * @brief Gestionnaire de fichiers .TAP pour l'Oric Atmos.
 * Implémente le "Fast Load" par injection directe en mémoire.
 */
class TapLoader {
public:
    /**
     * @brief Parse un fichier TAP et l'injecte dans la RAM.
     * @param filepath Chemin du fichier .tap
     * @param bus Le bus mémoire pour l'injection
     * @param cpu Le CPU pour la mise à jour du PC (si autorun)
     * @return True si succès.
     */
    static bool fastLoad(const std::string& filepath, MemoryBus* bus, Cpu6502* cpu);

private:
    struct TapHeader {
        uint8_t  type;       // 00=Basic, 80=Mcode
        uint8_t  autorun;    // 00=No, 01=Yes
        uint16_t endAddr;
        uint16_t startAddr;
        std::string name;
    };
};

#endif // TAPLOADER_H
