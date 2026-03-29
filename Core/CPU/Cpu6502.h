#ifndef CPU6502_H
#define CPU6502_H

#include <cstdint>
#include <cstdio>

class MemoryBus;

/**
 * @brief MOS 6502 Processor Emulator (Core)
 *
 * This class implements the logic for the 8-bit MOS Technology 6502 processor, 
 * originally running at 1 MHz in the Oric Atmos. It fetches and decodes instructions, 
 * handles all 13 addressing modes, and computes CPU flags (including full BCD support).
 * 
 * Design Choice:
 * For maximum performance, decoding is implemented via a gigantic switch/case statement
 * located in `Cpu6502.cpp` rather than relying on abstract classes or function pointers. 
 * This allows modern C++ compilers to optimize the decode loop into an extremely fast jump table.
 */
class Cpu6502 {
public:
    /**
     * @brief Processor Status Flags
     * Bit masks representing the state of the 6502.
     * 
     * The Status register (P) is an 8-bit register holding condition codes.
     */
    enum Flags {
        C = (1 << 0), ///< Carry Flag: Set if the last operation caused an overflow in bit 7 or an underflow.
        Z = (1 << 1), ///< Zero Flag: Set if the result of the last operation was zero.
        I = (1 << 2), ///< Interrupt Disable: Set to disable maskable interrupts (IRQ).
        D = (1 << 3), ///< Decimal Mode: Set if the CPU should use Binary Coded Decimal (BCD) arithmetic.
        B = (1 << 4), ///< Break Command: Set when a software interrupt (BRK) is executed.
        U = (1 << 5), ///< Unused (always 1).
        V = (1 << 6), ///< Overflow Flag: Set if signed arithmetic resulted in an incorrect sign bit.
        N = (1 << 7)  ///< Negative Flag: Set if the result of the last operation had bit 7 set.
    };

    /**
     * @brief Constructor
     * @param bus Pointer to the main memory bus to fetch instructions and memory-mapped IO.
     */
    Cpu6502(MemoryBus* bus);
    
    ~Cpu6502();
    
    /**
     * @brief Forces the CPU to its boot state.
     * Loads the reset vector from memory addresses $FFFC and $FFFD into the Program Counter.
     */
    void reset();
    
    /**
     * @brief Fetches, decodes, and executes exactly one instruction.
     * @return The number of CPU clock cycles consumed by the instruction.
     */
    int step(); 
    
    /**
     * @brief Indique si le CPU a craché sur un Opcode Inconnu (Sert de test).
     */
    bool isHalted() const { return m_halted; }

    /**
     * @brief Tirer la broche IRQ vers le haut ou le bas (Signal matériel du VIA).
     */
    void setIrqLine(bool active) { m_irqRequested = active; }
    
    /**
     * @brief Modifie manuellement le compteur de programme (utilisé par le TapLoader).
     */
    void setPC(uint16_t newPC) { PC = newPC; }
    
private:
    MemoryBus* m_bus;

    // --- CPU Registers ---
    uint16_t PC;    ///< Program Counter
    uint8_t A;      ///< Accumulator (Core computational register)
    uint8_t X;      ///< Index Register X
    uint8_t Y;      ///< Index Register Y
    uint8_t SP;     ///< Stack Pointer (Page 1: $0100 - $01FF)
    uint8_t Status; ///< Processor Status (Flags)
    bool m_halted = false; // Flag for debug crash
    bool m_irqRequested = false; // Pin d'interruption externe
    int m_debugLogCount = 0;   // Diagnostic : Compteur de logs après RESET
    FILE* m_logFile = nullptr; // Fichier de log de trace

    // --- Flag Manipulations ---
    
    /**
     * @brief Sets or clears a specific flag in the status register.
     * @param flag The flag to modify (e.g. Cpu6502::Z).
     * @param value True to set the flag to 1, false to clear it to 0.
     */
    void setFlag(Flags flag, bool value);
    
    /**
     * @brief Checks if a specific flag is currently set.
     * @param flag The flag to check.
     * @return True if the flag is 1, false otherwise.
     */
    bool getFlag(Flags flag) const;

    // --- Memory and Stack Helpers ---
    
    uint8_t fetchByte();
    uint16_t fetchWord();
    
    /**
     * @brief Pushes a single byte onto the hardware stack.
     * The 6502 stack is located in Page 1, starting at $01FF and growing downwards.
     * @param data The 8-bit value to push.
     */
    void pushByte(uint8_t data);
    
    /**
     * @brief Pops a single byte from the hardware stack.
     * @return The 8-bit value popped.
     */
    uint8_t popByte();
    
    void pushWord(uint16_t data);
    uint16_t popWord();

    // --- Addressing Modes ---
    // These functions resolve the Effective Address (EA) where operands are located.
    // They may return additional cycles if page boundaries are crossed.
    
    uint16_t addrZeroPage();
    uint16_t addrZeroPageX();
    uint16_t addrZeroPageY();
    uint16_t addrAbsolute();
    uint16_t addrAbsoluteX(bool& pageCrossed);
    uint16_t addrAbsoluteY(bool& pageCrossed);
    uint16_t addrIndirect();
    uint16_t addrIndirectX();
    uint16_t addrIndirectY(bool& pageCrossed);
    
    // --- Instruction Handlers ---
    // Helper handlers for complex instructions like ADC/SBC
    void opADC(uint8_t operand);
    void opSBC(uint8_t operand);
};

#endif // CPU6502_H
