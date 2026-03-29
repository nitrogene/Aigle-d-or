#ifndef MEMORYBUS_H
#define MEMORYBUS_H

#include <cstdint>
#include <vector>

class Via; // Forward Declaration du 6522
class AySound;

class MemoryBus {
public:
    MemoryBus();
    ~MemoryBus();

    /**
     * @brief Connecte les contrôleurs matériels à la carte mère.
     */
    void attachVia(Via* via) { m_via = via; }
    void attachAySound(AySound* ay) { m_ay = ay; }

    /**
     * @brief Accès aux composants pour la GUI
     */
    AySound* getAySound() const { return m_ay; }

    /**
     * @brief Lecture d'un octet sur le bus.
     */
    uint8_t read(uint16_t addr);

    /**
     * @brief Écriture d'un octet sur le bus.
     */
    void write(uint16_t addr, uint8_t data);

    /**
     * @brief Remplissage complet de la ROM d'un coup.
     */
    bool loadRom(const uint8_t* romData, size_t size);

private:
    std::vector<uint8_t> m_ram; ///< 64 Ko de RAM unifiée
    std::vector<uint8_t> m_rom; ///< 16 Ko de ROM BIOS
    
    Via* m_via = nullptr;       ///< Composant matériel d'interface
    AySound* m_ay = nullptr;    ///< Processeur sonore et clavier
};

#endif // MEMORYBUS_H
