#ifndef ULA_H
#define ULA_H

#include <vector>
#include <cstdint>

class MemoryBus;

/**
 * @brief ULA (Uncommitted Logic Array) Video Processor
 * 
 * Generates the Oric Atmos 240x224 RGB Frame Buffer at 50Hz.
 * It reads the Video RAM attributes (Ink/Paper) and characters.
 */
class Ula {
public:
    Ula(MemoryBus* bus);
    ~Ula();

    /**
     * @brief Scans Video RAM and renders a complete 240x224 frame.
     */
    void renderFrame();

    /**
     * @brief Retrieves the raw ARGB32 frame buffer.
     */
    const std::vector<uint32_t>& getFrameBuffer() const;

    static const int SCREEN_WIDTH = 240;
    static const int SCREEN_HEIGHT = 224;

private:
    MemoryBus* m_bus;
    std::vector<uint32_t> m_frameBuffer;

    // Native Oric Palette (8 colors)
    uint32_t m_palette[8];
};

#endif // ULA_H
