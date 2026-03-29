#include "Ula.h"
#include "MemoryBus.h"

Ula::Ula(MemoryBus* bus) : m_bus(bus) {
    // Initialization of the 240x224 ARGB32 Frame Buffer (Black background)
    m_frameBuffer.resize(SCREEN_WIDTH * SCREEN_HEIGHT, 0xFF000000);

    // Oric Hardware Palette
    m_palette[0] = 0xFF000000; // Black
    m_palette[1] = 0xFFFF0000; // Red
    m_palette[2] = 0xFF00FF00; // Green
    m_palette[3] = 0xFFFFFF00; // Yellow
    m_palette[4] = 0xFF0000FF; // Blue
    m_palette[5] = 0xFFFF00FF; // Magenta
    m_palette[6] = 0xFF00FFFF; // Cyan
    m_palette[7] = 0xFFFFFFFF; // White
}

Ula::~Ula() {}

void Ula::renderFrame() {
    if (!m_bus) return;

    // Text Mode: VRAM $BB80–$BFDF (40 colonnes x 28 lignes)
    // La ROM Atmos initialise cet espace au boot avant d'afficher le prompt.
    const uint16_t videoBase = 0xBB80;

    // La ROM copie la font en $B600 pendant son initialisation.
    // Ne pas appeler renderFrame() avant que la ROM ait eu le temps de le faire
    // (typiquement ~50 000 cycles après le RESET).
    const uint16_t fontBase  = 0xB600;

    for (int row = 0; row < 28; ++row) {
        // Attributs série : réinitialisés à blanc sur noir en début de chaque ligne
        uint32_t currentPaper = m_palette[0]; // Black
        uint32_t currentInk   = m_palette[7]; // White

        for (int col = 0; col < 40; ++col) {
            uint16_t address = videoBase + (row * 40) + col;
            uint8_t cell = m_bus->read(address);

            if (cell <= 0x1F) {
                // Attribut série (caractère de contrôle)
                if (cell <= 0x07) {
                    currentInk = m_palette[cell & 0x07];
                } else if (cell >= 0x10 && cell <= 0x17) {
                    currentPaper = m_palette[cell & 0x07];
                }
                // La cellule attribut s'affiche comme un espace couleur paper
                for (int y = 0; y < 8; ++y) {
                    for (int x = 0; x < 6; ++x) {
                        m_frameBuffer[(row * 8 + y) * SCREEN_WIDTH + (col * 6 + x)] = currentPaper;
                    }
                }
            } else {
                // Caractère imprimable ($20–$7F), bit7 = vidéo inverse
                bool isInverse  = (cell & 0x80) != 0;
                uint8_t charIdx = cell & 0x7F;

                for (int y = 0; y < 8; ++y) {
                    uint8_t fontByte = m_bus->read(fontBase + (charIdx * 8) + y);

                    for (int x = 0; x < 6; ++x) {
                        // Bit5 = pixel gauche, bit0 = pixel droit (6 pixels/caractère)
                        bool pixelOn = (fontByte & (1 << (5 - x))) != 0;
                        if (isInverse) pixelOn = !pixelOn;

                        m_frameBuffer[(row * 8 + y) * SCREEN_WIDTH + (col * 6 + x)] =
                            pixelOn ? currentInk : currentPaper;
                    }
                }
            }
        }
    }
}

const std::vector<uint32_t>& Ula::getFrameBuffer() const {
    return m_frameBuffer;
}
