#include "Cpu6502.h"
#include "../Hardware/MemoryBus.h"
#include <iostream>
#include <cstdio>

Cpu6502::Cpu6502(MemoryBus* bus) : m_bus(bus) {
    reset();
}

Cpu6502::~Cpu6502() {
    if (m_logFile) std::fclose(m_logFile);
}

void Cpu6502::setFlag(Flags flag, bool value) {
    if (value) Status |= flag; else Status &= ~flag;
}

bool Cpu6502::getFlag(Flags flag) const {
    return (Status & flag) != 0;
}

void Cpu6502::reset() {
    if (!m_bus) return;
    setFlag(I, true);      // On coupe les interruptions au démarrage
    PC = (m_bus->read(0xFFFD) << 8) | m_bus->read(0xFFFC);
    A = X = Y = 0;
    SP = 0xFD;
    Status = I | U; // U (bit 5) est toujours à 1 sur 6502
    m_halted = false;
    m_irqRequested = false;
    m_debugLogCount = 1000; // On loggue les 1000 premières instructions dans le fichier
    
    if (!m_logFile) {
        m_logFile = std::fopen("trace.log", "w");
    }
}

uint8_t Cpu6502::fetchByte() { return m_bus->read(PC++); }

uint16_t Cpu6502::fetchWord() {
    uint8_t low = fetchByte();
    uint8_t high = fetchByte();
    return (high << 8) | low;
}

void Cpu6502::pushByte(uint8_t data) {
    m_bus->write(0x0100 + SP, data);
    SP--;
}

uint8_t Cpu6502::popByte() {
    SP++;
    return m_bus->read(0x0100 + SP);
}

void Cpu6502::pushWord(uint16_t data) {
    pushByte((data >> 8) & 0xFF);
    pushByte(data & 0xFF);
}

uint16_t Cpu6502::popWord() {
    uint8_t low = popByte();
    uint8_t high = popByte();
    return (high << 8) | low;
}

uint16_t Cpu6502::addrZeroPage() { return fetchByte(); }
uint16_t Cpu6502::addrZeroPageX() { return (fetchByte() + X) & 0xFF; }
uint16_t Cpu6502::addrZeroPageY() { return (fetchByte() + Y) & 0xFF; }
uint16_t Cpu6502::addrAbsolute() { return fetchWord(); }
uint16_t Cpu6502::addrAbsoluteX(bool& pc) { uint16_t b=fetchWord(); uint16_t a=b+X; pc=((a&0xFF00)!=(b&0xFF00)); return a; }
uint16_t Cpu6502::addrAbsoluteY(bool& pc) { uint16_t b=fetchWord(); uint16_t a=b+Y; pc=((a&0xFF00)!=(b&0xFF00)); return a; }
uint16_t Cpu6502::addrIndirect() { 
    uint16_t ptr = fetchWord(); 
    if((ptr & 0x00FF) == 0x00FF) return (m_bus->read(ptr & 0xFF00) << 8) | m_bus->read(ptr);
    return (m_bus->read(ptr + 1) << 8) | m_bus->read(ptr);
}
uint16_t Cpu6502::addrIndirectX() { 
    uint8_t zp = fetchByte(); 
    uint8_t ptr = (zp + X) & 0xFF;
    uint8_t lo = m_bus->read(ptr);
    uint8_t hi = m_bus->read((ptr + 1) & 0xFF);
    return (hi << 8) | lo; 
}
uint16_t Cpu6502::addrIndirectY(bool& pc) { 
    uint8_t zp = fetchByte(); 
    uint16_t lo = m_bus->read(zp);
    uint16_t hi = m_bus->read((zp + 1) & 0xFF);
    uint16_t base = (hi << 8) | lo;
    uint16_t addr = base + Y;
    pc = ((addr & 0xFF00) != (base & 0xFF00));
    return addr; 
}

void Cpu6502::opADC(uint8_t operand) {
    if (getFlag(D)) {
        uint16_t val1 = A & 0x0F, val2 = operand & 0x0F, val3 = getFlag(C) ? 1 : 0;
        uint16_t res = val1 + val2 + val3;
        if (res > 0x09) res += 0x06;
        uint16_t finalRes = (A & 0xF0) + (operand & 0xF0) + (res & 0xF0) + (res & 0x0F);
        setFlag(Z, (A + operand + val3) == 0);
        setFlag(N, (finalRes & 0x80) != 0);
        setFlag(V, ((A ^ finalRes) & ~(A ^ operand) & 0x80) != 0);
        if (finalRes > 0x90) finalRes += 0x60;
        setFlag(C, (finalRes & 0xFF00) > 0);
        A = finalRes & 0xFF;
    } else {
        uint16_t res = A + operand + (getFlag(C) ? 1 : 0);
        setFlag(C, res > 0xFF); setFlag(Z, (res & 0xFF) == 0); setFlag(N, (res & 0x80) != 0);
        setFlag(V, (~(A ^ operand) & (A ^ res) & 0x80) != 0);
        A = res & 0xFF;
    }
}

void Cpu6502::opSBC(uint8_t operand) {
    if (getFlag(D)) {
        uint16_t diff = A - operand - (getFlag(C) ? 0 : 1);
        int16_t al = (A & 0x0F) - (operand & 0x0F) - (getFlag(C) ? 0 : 1);
        int16_t ah = (A >> 4) - (operand >> 4);
        if (al < 0) { al -= 6; ah--; }
        if (ah < 0) { ah -= 6; }
        setFlag(C, diff < 0x100);
        setFlag(V, ((A ^ diff) & (A ^ operand) & 0x80) != 0);
        setFlag(Z, (diff & 0xFF) == 0); setFlag(N, (diff & 0x80) != 0);
        A = ((ah << 4) | (al & 0x0F)) & 0xFF;
    } else {
        uint16_t res = A + (~operand & 0xFF) + (getFlag(C) ? 1 : 0);
        setFlag(C, res > 0xFF); setFlag(Z, (res & 0xFF) == 0); setFlag(N, (res & 0x80) != 0);
        setFlag(V, ((A ^ res) & (~operand ^ res) & 0x80) != 0);
        A = res & 0xFF;
    }
}

int Cpu6502::step() {
    if (m_halted || !m_bus) return 0;

    // --- Diagnostic Log ---
    if (m_debugLogCount > 0) {
        uint8_t op = m_bus->read(PC);
        char f[9];
        f[0] = getFlag(N) ? 'N' : '.';
        f[1] = getFlag(V) ? 'V' : '.';
        f[2] = getFlag(U) ? 'U' : '.';
        f[3] = getFlag(B) ? 'B' : '.';
        f[4] = getFlag(D) ? 'D' : '.';
        f[5] = getFlag(I) ? 'I' : '.';
        f[6] = getFlag(Z) ? 'Z' : '.';
        f[7] = getFlag(C) ? 'C' : '.';
        f[8] = '\0';
        
        std::printf("[Trace] PC:%04X OP:%02X  A:%02X X:%02X Y:%02X SP:%02X F:%s\n", 
                    PC, op, A, X, Y, SP, f);
        
        if (m_logFile) {
            std::fprintf(m_logFile, "[Trace] PC:%04X OP:%02X  A:%02X X:%02X Y:%02X SP:%02X F:%s\n", 
                        PC, op, A, X, Y, SP, f);
            std::fflush(m_logFile);
        }
        
        m_debugLogCount--;
    }

    // --- Interruptions matérielles (IRQ) ---
    // Routine matérielle d'interruption : le VIA 6522 a levé le Signal IRQ
    if (m_irqRequested && !getFlag(I)) {
        pushWord(PC); // On sauvegarde notre adresse courante
        pushByte(Status & ~B); // Le flag B est à 0 (c'est une IRQ hardware, pas logicielle)
        setFlag(I, true);      // On coupe les futures interruptions récursives
        PC = (m_bus->read(0xFFFF) << 8) | m_bus->read(0xFFFE); // Saut vers la routine d'IRQ du système Oric !
        return 7; // Cycle hardware pris pour un IRQ Dispatch standard.
    }

    uint8_t op = fetchByte();
    int c = 2; uint16_t ea = 0; bool pcx = false;

    // Helper lambdas
    auto LDA = [&](uint8_t v) { A = v; setFlag(Z, A == 0); setFlag(N, (A & 0x80) != 0); };
    auto CMP = [&](uint8_t a, uint8_t v) { uint16_t r = a - v; setFlag(C, a >= v); setFlag(Z, (r & 0xFF) == 0); setFlag(N, (r & 0x80) != 0); };
    auto AND = [&](uint8_t v) { A &= v; setFlag(Z, A == 0); setFlag(N, (A & 0x80) != 0); };
    auto ORA = [&](uint8_t v) { A |= v; setFlag(Z, A == 0); setFlag(N, (A & 0x80) != 0); };
    auto EOR = [&](uint8_t v) { A ^= v; setFlag(Z, A == 0); setFlag(N, (A & 0x80) != 0); };
    auto BIT = [&](uint8_t v) { setFlag(Z, (A & v) == 0); setFlag(N, (v & 0x80) != 0); setFlag(V, (v & 0x40) != 0); };
    auto ASL = [&](uint8_t v) { setFlag(C, (v & 0x80) != 0); v <<= 1; setFlag(Z, v == 0); setFlag(N, (v & 0x80) != 0); return v; };
    auto LSR = [&](uint8_t v) { setFlag(C, (v & 0x01) != 0); v >>= 1; setFlag(Z, v == 0); setFlag(N, false); return v; };
    auto ROL = [&](uint8_t v) { bool nc = (v & 0x80) != 0; v = (v << 1) | (getFlag(C) ? 1 : 0); setFlag(C, nc); setFlag(Z, v == 0); setFlag(N, (v & 0x80) != 0); return v; };
    auto ROR = [&](uint8_t v) { bool nc = (v & 0x01) != 0; v = (v >> 1) | (getFlag(C) ? 0x80 : 0); setFlag(C, nc); setFlag(Z, v == 0); setFlag(N, (v & 0x80) != 0); return v; };
    auto BRANCH = [&](bool cond) { int8_t off = fetchByte(); if (cond) { PC += off; c = 3; } else { c = 2; } };

    switch (op) {
        // --- LDA ---
        case 0xA9: LDA(fetchByte()); c = 2; break;
        case 0xA5: LDA(m_bus->read(addrZeroPage())); c = 3; break;
        case 0xB5: LDA(m_bus->read(addrZeroPageX())); c = 4; break;
        case 0xAD: LDA(m_bus->read(addrAbsolute())); c = 4; break;
        case 0xBD: {
            uint16_t base = m_bus->read(PC) | (m_bus->read(PC+1) << 8);
            uint16_t target = addrAbsoluteX(pcx);
            uint8_t val = m_bus->read(target);
            LDA(val);
            c = 4 + pcx;
            const char* msg = "LDA abs,X : base=%04X X=%02X ea=%04X val=%02X\n";
            std::printf(msg, base, X, target, val);
            if (m_logFile) {
                std::fprintf(m_logFile, msg, base, X, target, val);
                std::fflush(m_logFile);
            }
            break;
        }        case 0xB9: LDA(m_bus->read(addrAbsoluteY(pcx))); c = 4 + pcx; break;
        case 0xA1: LDA(m_bus->read(addrIndirectX())); c = 6; break;
        case 0xB1: LDA(m_bus->read(addrIndirectY(pcx))); c = 5 + pcx; break;

        // --- STA ---
        case 0x85: m_bus->write(addrZeroPage(), A); c = 3; break;
        case 0x95: m_bus->write(addrZeroPageX(), A); c = 4; break;
        case 0x8D: m_bus->write(addrAbsolute(), A); c = 4; break;
        case 0x9D: m_bus->write(addrAbsoluteX(pcx), A); c = 5; break;
        case 0x99: m_bus->write(addrAbsoluteY(pcx), A); c = 5; break;
        case 0x81: m_bus->write(addrIndirectX(), A); c = 6; break;
        case 0x91: m_bus->write(addrIndirectY(pcx), A); c = 6; break;

        // --- LDX ---
        case 0xA2: X = fetchByte(); setFlag(Z, X == 0); setFlag(N, (X & 0x80) != 0); c = 2; break;
        case 0xA6: X = m_bus->read(addrZeroPage()); setFlag(Z, X == 0); setFlag(N, (X & 0x80) != 0); c = 3; break;
        case 0xB6: X = m_bus->read(addrZeroPageY()); setFlag(Z, X == 0); setFlag(N, (X & 0x80) != 0); c = 4; break;
        case 0xAE: X = m_bus->read(addrAbsolute()); setFlag(Z, X == 0); setFlag(N, (X & 0x80) != 0); c = 4; break;
        case 0xBE: X = m_bus->read(addrAbsoluteY(pcx)); setFlag(Z, X == 0); setFlag(N, (X & 0x80) != 0); c = 4 + pcx; break;

        // --- LDY ---
        case 0xA0: Y = fetchByte(); setFlag(Z, Y == 0); setFlag(N, (Y & 0x80) != 0); c = 2; break;
        case 0xA4: Y = m_bus->read(addrZeroPage()); setFlag(Z, Y == 0); setFlag(N, (Y & 0x80) != 0); c = 3; break;
        case 0xB4: Y = m_bus->read(addrZeroPageX()); setFlag(Z, Y == 0); setFlag(N, (Y & 0x80) != 0); c = 4; break;
        case 0xAC: Y = m_bus->read(addrAbsolute()); setFlag(Z, Y == 0); setFlag(N, (Y & 0x80) != 0); c = 4; break;
        case 0xBC: Y = m_bus->read(addrAbsoluteX(pcx)); setFlag(Z, Y == 0); setFlag(N, (Y & 0x80) != 0); c = 4 + pcx; break;

        // --- STX & STY ---
        case 0x86: m_bus->write(addrZeroPage(), X); c = 3; break;
        case 0x96: m_bus->write(addrZeroPageY(), X); c = 4; break;
        case 0x8E: m_bus->write(addrAbsolute(), X); c = 4; break;
        case 0x84: m_bus->write(addrZeroPage(), Y); c = 3; break;
        case 0x94: m_bus->write(addrZeroPageX(), Y); c = 4; break;
        case 0x8C: m_bus->write(addrAbsolute(), Y); c = 4; break;

        // --- TAX, TAY, TXA, TYA, TSX, TXS ---
        case 0xAA: X = A; setFlag(Z, X == 0); setFlag(N, (X & 0x80) != 0); c = 2; break;
        case 0xA8: Y = A; setFlag(Z, Y == 0); setFlag(N, (Y & 0x80) != 0); c = 2; break;
        case 0x8A: A = X; setFlag(Z, A == 0); setFlag(N, (A & 0x80) != 0); c = 2; break;
        case 0x98: A = Y; setFlag(Z, A == 0); setFlag(N, (A & 0x80) != 0); c = 2; break;
        case 0x9A: SP = X; c = 2; break;
        case 0xBA: X = SP; setFlag(Z, X == 0); setFlag(N, (X & 0x80) != 0); c = 2; break;

        // --- ADC / SBC ---
        case 0x69: opADC(fetchByte()); c = 2; break;
        case 0x65: opADC(m_bus->read(addrZeroPage())); c = 3; break;
        case 0x75: opADC(m_bus->read(addrZeroPageX())); c = 4; break;
        case 0x6D: opADC(m_bus->read(addrAbsolute())); c = 4; break;
        case 0x7D: opADC(m_bus->read(addrAbsoluteX(pcx))); c = 4 + pcx; break;
        case 0x79: opADC(m_bus->read(addrAbsoluteY(pcx))); c = 4 + pcx; break;
        case 0x61: opADC(m_bus->read(addrIndirectX())); c = 6; break;
        case 0x71: opADC(m_bus->read(addrIndirectY(pcx))); c = 5 + pcx; break;

        case 0xE9: opSBC(fetchByte()); c = 2; break;
        case 0xE5: opSBC(m_bus->read(addrZeroPage())); c = 3; break;
        case 0xF5: opSBC(m_bus->read(addrZeroPageX())); c = 4; break;
        case 0xED: opSBC(m_bus->read(addrAbsolute())); c = 4; break;
        case 0xFD: opSBC(m_bus->read(addrAbsoluteX(pcx))); c = 4 + pcx; break;
        case 0xF9: opSBC(m_bus->read(addrAbsoluteY(pcx))); c = 4 + pcx; break;
        case 0xE1: opSBC(m_bus->read(addrIndirectX())); c = 6; break;
        case 0xF1: opSBC(m_bus->read(addrIndirectY(pcx))); c = 5 + pcx; break;

        // --- AND, ORA, EOR, BIT ---
        case 0x29: AND(fetchByte()); c = 2; break;
        case 0x25: AND(m_bus->read(addrZeroPage())); c = 3; break;
        case 0x35: AND(m_bus->read(addrZeroPageX())); c = 4; break;
        case 0x2D: AND(m_bus->read(addrAbsolute())); c = 4; break;
        case 0x3D: AND(m_bus->read(addrAbsoluteX(pcx))); c = 4 + pcx; break;
        case 0x39: AND(m_bus->read(addrAbsoluteY(pcx))); c = 4 + pcx; break;
        case 0x21: AND(m_bus->read(addrIndirectX())); c = 6; break;
        case 0x31: AND(m_bus->read(addrIndirectY(pcx))); c = 5 + pcx; break;

        case 0x09: ORA(fetchByte()); c = 2; break;
        case 0x05: ORA(m_bus->read(addrZeroPage())); c = 3; break;
        case 0x15: ORA(m_bus->read(addrZeroPageX())); c = 4; break;
        case 0x0D: ORA(m_bus->read(addrAbsolute())); c = 4; break;
        case 0x1D: ORA(m_bus->read(addrAbsoluteX(pcx))); c = 4 + pcx; break;
        case 0x19: ORA(m_bus->read(addrAbsoluteY(pcx))); c = 4 + pcx; break;
        case 0x01: ORA(m_bus->read(addrIndirectX())); c = 6; break;
        case 0x11: ORA(m_bus->read(addrIndirectY(pcx))); c = 5 + pcx; break;

        case 0x49: EOR(fetchByte()); c = 2; break;
        case 0x45: EOR(m_bus->read(addrZeroPage())); c = 3; break;
        case 0x55: EOR(m_bus->read(addrZeroPageX())); c = 4; break;
        case 0x4D: EOR(m_bus->read(addrAbsolute())); c = 4; break;
        case 0x5D: EOR(m_bus->read(addrAbsoluteX(pcx))); c = 4 + pcx; break;
        case 0x59: EOR(m_bus->read(addrAbsoluteY(pcx))); c = 4 + pcx; break;
        case 0x41: EOR(m_bus->read(addrIndirectX())); c = 6; break;
        case 0x51: EOR(m_bus->read(addrIndirectY(pcx))); c = 5 + pcx; break;

        case 0x24: BIT(m_bus->read(addrZeroPage())); c = 3; break;
        case 0x2C: BIT(m_bus->read(addrAbsolute())); c = 4; break;

        // --- CMP, CPX, CPY ---
        case 0xC9: CMP(A, fetchByte()); c = 2; break;
        case 0xC5: CMP(A, m_bus->read(addrZeroPage())); c = 3; break;
        case 0xD5: CMP(A, m_bus->read(addrZeroPageX())); c = 4; break;
        case 0xCD: CMP(A, m_bus->read(addrAbsolute())); c = 4; break;
        case 0xDD: CMP(A, m_bus->read(addrAbsoluteX(pcx))); c = 4 + pcx; break;
        case 0xD9: CMP(A, m_bus->read(addrAbsoluteY(pcx))); c = 4 + pcx; break;
        case 0xC1: CMP(A, m_bus->read(addrIndirectX())); c = 6; break;
        case 0xD1: CMP(A, m_bus->read(addrIndirectY(pcx))); c = 5 + pcx; break;

        case 0xE0: CMP(X, fetchByte()); c = 2; break;
        case 0xE4: CMP(X, m_bus->read(addrZeroPage())); c = 3; break;
        case 0xEC: CMP(X, m_bus->read(addrAbsolute())); c = 4; break;

        case 0xC0: CMP(Y, fetchByte()); c = 2; break;
        case 0xC4: CMP(Y, m_bus->read(addrZeroPage())); c = 3; break;
        case 0xCC: CMP(Y, m_bus->read(addrAbsolute())); c = 4; break;

        // --- INC, DEC ---
        case 0xE6: ea=addrZeroPage(); { uint8_t v = m_bus->read(ea) + 1; m_bus->write(ea, v); setFlag(Z, v==0); setFlag(N, (v&0x80)!=0); c=5; } break;
        case 0xF6: ea=addrZeroPageX(); { uint8_t v = m_bus->read(ea) + 1; m_bus->write(ea, v); setFlag(Z, v==0); setFlag(N, (v&0x80)!=0); c=6; } break;
        case 0xEE: ea=addrAbsolute(); { uint8_t v = m_bus->read(ea) + 1; m_bus->write(ea, v); setFlag(Z, v==0); setFlag(N, (v&0x80)!=0); c=6; } break;
        case 0xFE: ea=addrAbsoluteX(pcx); { uint8_t v = m_bus->read(ea) + 1; m_bus->write(ea, v); setFlag(Z, v==0); setFlag(N, (v&0x80)!=0); c=7; } break;

        case 0xC6: ea=addrZeroPage(); { uint8_t v = m_bus->read(ea) - 1; m_bus->write(ea, v); setFlag(Z, v==0); setFlag(N, (v&0x80)!=0); c=5; } break;
        case 0xD6: ea=addrZeroPageX(); { uint8_t v = m_bus->read(ea) - 1; m_bus->write(ea, v); setFlag(Z, v==0); setFlag(N, (v&0x80)!=0); c=6; } break;
        case 0xCE: ea=addrAbsolute(); { uint8_t v = m_bus->read(ea) - 1; m_bus->write(ea, v); setFlag(Z, v==0); setFlag(N, (v&0x80)!=0); c=6; } break;
        case 0xDE: ea=addrAbsoluteX(pcx); { uint8_t v = m_bus->read(ea) - 1; m_bus->write(ea, v); setFlag(Z, v==0); setFlag(N, (v&0x80)!=0); c=7; } break;

        case 0xE8: X++; setFlag(Z, X == 0); setFlag(N, (X & 0x80) != 0); c = 2; break; // INX
        case 0xC8: Y++; setFlag(Z, Y == 0); setFlag(N, (Y & 0x80) != 0); c = 2; break; // INY
        case 0xCA: X--; setFlag(Z, X == 0); setFlag(N, (X & 0x80) != 0); c = 2; break; // DEX
        case 0x88: Y--; setFlag(Z, Y == 0); setFlag(N, (Y & 0x80) != 0); c = 2; break; // DEY

        // --- Shifts ---
        case 0x0A: A = ASL(A); c = 2; break;
        case 0x06: ea=addrZeroPage(); m_bus->write(ea, ASL(m_bus->read(ea))); c=5; break;
        case 0x16: ea=addrZeroPageX(); m_bus->write(ea, ASL(m_bus->read(ea))); c=6; break;
        case 0x0E: ea=addrAbsolute(); m_bus->write(ea, ASL(m_bus->read(ea))); c=6; break;
        case 0x1E: ea=addrAbsoluteX(pcx); m_bus->write(ea, ASL(m_bus->read(ea))); c=7; break;

        case 0x4A: A = LSR(A); c = 2; break;
        case 0x46: ea=addrZeroPage(); m_bus->write(ea, LSR(m_bus->read(ea))); c=5; break;
        case 0x56: ea=addrZeroPageX(); m_bus->write(ea, LSR(m_bus->read(ea))); c=6; break;
        case 0x4E: ea=addrAbsolute(); m_bus->write(ea, LSR(m_bus->read(ea))); c=6; break;
        case 0x5E: ea=addrAbsoluteX(pcx); m_bus->write(ea, LSR(m_bus->read(ea))); c=7; break;

        case 0x2A: A = ROL(A); c = 2; break;
        case 0x26: ea=addrZeroPage(); m_bus->write(ea, ROL(m_bus->read(ea))); c=5; break;
        case 0x36: ea=addrZeroPageX(); m_bus->write(ea, ROL(m_bus->read(ea))); c=6; break;
        case 0x2E: ea=addrAbsolute(); m_bus->write(ea, ROL(m_bus->read(ea))); c=6; break;
        case 0x3E: ea=addrAbsoluteX(pcx); m_bus->write(ea, ROL(m_bus->read(ea))); c=7; break;

        case 0x6A: A = ROR(A); c = 2; break;
        case 0x66: ea=addrZeroPage(); m_bus->write(ea, ROR(m_bus->read(ea))); c=5; break;
        case 0x76: ea=addrZeroPageX(); m_bus->write(ea, ROR(m_bus->read(ea))); c=6; break;
        case 0x6E: ea=addrAbsolute(); m_bus->write(ea, ROR(m_bus->read(ea))); c=6; break;
        case 0x7E: ea=addrAbsoluteX(pcx); m_bus->write(ea, ROR(m_bus->read(ea))); c=7; break;

        // --- Stack ---
        case 0x48: pushByte(A); c=3; break; // PHA
        case 0x08: pushByte(Status | B | U); c=3; break; // PHP: Pousse avec B et U à 1
        case 0x68: A = popByte(); setFlag(Z, A == 0); setFlag(N, (A & 0x80) != 0); c=4; break; // PLA
        case 0x28: Status = (popByte() & ~B) | U; c=4; break; // PLP: Ignore B du stack, force U à 1

        // --- Branches ---
        case 0x90: BRANCH(!getFlag(C)); break;
        case 0xB0: BRANCH(getFlag(C)); break;
        case 0xF0: BRANCH(getFlag(Z)); break;
        case 0xD0: BRANCH(!getFlag(Z)); break;
        case 0x10: BRANCH(!getFlag(N)); break;
        case 0x30: BRANCH(getFlag(N)); break;
        case 0x50: BRANCH(!getFlag(V)); break;
        case 0x70: BRANCH(getFlag(V)); break;

        // --- Jumps ---
        case 0x4C: PC = addrAbsolute(); c = 3; break;
        case 0x6C: PC = addrIndirect(); c = 5; break;
        case 0x20: ea = addrAbsolute(); pushWord(PC - 1); PC = ea; c = 6; break; // JSR
        case 0x60: PC = popWord() + 1; c = 6; break; // RTS
        case 0x40: Status = (popByte() & ~B) | U; PC = popWord(); c = 6; break; // RTI: Ignore B, force U

        // --- Flags ---
        case 0x18: setFlag(C, false); c=2; break;
        case 0x38: setFlag(C, true);  c=2; break;
        case 0x58: setFlag(I, false); c=2; break;
        case 0x78: setFlag(I, true);  c=2; break;
        case 0xB8: setFlag(V, false); c=2; break;
        case 0xD8: setFlag(D, false); c=2; break;
        case 0xF8: setFlag(D, true);  c=2; break;

        // --- System ---
        case 0xEA: c = 2; break; // NOP
        case 0x00: // BRK
            PC++; // Skip padding byte
            pushWord(PC);
            pushByte(Status | B | U);
            setFlag(I, true);
            PC = (m_bus->read(0xFFFF) << 8) | m_bus->read(0xFFFE);
            c = 7;
            break;

        default:
            std::cerr << "[CPU CRASH] Unhandled opcode : 0x" << std::hex << (int)op << " at PC=0x" << (PC - 1) << std::endl;
            m_halted = true;
            c = 0;
            break;
    }
    return c;
}
