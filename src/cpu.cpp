#include "cpu.hpp"
#include "memory.hpp"

// =============================================================================
// Instruction Table (all 256 opcodes)
// =============================================================================

const std::array<CPU::Instruction, 256> CPU::INSTRUCTION_TABLE = {{
    // 0x00-0x0F
    {"BRK", &CPU::BRK, &CPU::addr_IMP, 7}, {"ORA", &CPU::ORA, &CPU::addr_IZX, 6},
    {"???", &CPU::XXX, &CPU::addr_IMP, 2}, {"SLO", &CPU::SLO, &CPU::addr_IZX, 8},
    {"NOP", &CPU::NOP, &CPU::addr_ZPG, 3}, {"ORA", &CPU::ORA, &CPU::addr_ZPG, 3},
    {"ASL", &CPU::ASL, &CPU::addr_ZPG, 5}, {"SLO", &CPU::SLO, &CPU::addr_ZPG, 5},
    {"PHP", &CPU::PHP, &CPU::addr_IMP, 3}, {"ORA", &CPU::ORA, &CPU::addr_IMM, 2},
    {"ASL", &CPU::ASL, &CPU::addr_ACC, 2}, {"???", &CPU::XXX, &CPU::addr_IMP, 2},
    {"NOP", &CPU::NOP, &CPU::addr_ABS, 4}, {"ORA", &CPU::ORA, &CPU::addr_ABS, 4},
    {"ASL", &CPU::ASL, &CPU::addr_ABS, 6}, {"SLO", &CPU::SLO, &CPU::addr_ABS, 6},
    
    // 0x10-0x1F
    {"BPL", &CPU::BPL, &CPU::addr_REL, 2}, {"ORA", &CPU::ORA, &CPU::addr_IZY, 5},
    {"???", &CPU::XXX, &CPU::addr_IMP, 2}, {"SLO", &CPU::SLO, &CPU::addr_IZY, 8},
    {"NOP", &CPU::NOP, &CPU::addr_ZPX, 4}, {"ORA", &CPU::ORA, &CPU::addr_ZPX, 4},
    {"ASL", &CPU::ASL, &CPU::addr_ZPX, 6}, {"SLO", &CPU::SLO, &CPU::addr_ZPX, 6},
    {"CLC", &CPU::CLC, &CPU::addr_IMP, 2}, {"ORA", &CPU::ORA, &CPU::addr_ABY, 4},
    {"NOP", &CPU::NOP, &CPU::addr_IMP, 2}, {"SLO", &CPU::SLO, &CPU::addr_ABY, 7},
    {"NOP", &CPU::NOP, &CPU::addr_ABX, 4}, {"ORA", &CPU::ORA, &CPU::addr_ABX, 4},
    {"ASL", &CPU::ASL, &CPU::addr_ABX, 7}, {"SLO", &CPU::SLO, &CPU::addr_ABX, 7},
    
    // 0x20-0x2F
    {"JSR", &CPU::JSR, &CPU::addr_ABS, 6}, {"AND", &CPU::AND, &CPU::addr_IZX, 6},
    {"???", &CPU::XXX, &CPU::addr_IMP, 2}, {"RLA", &CPU::RLA, &CPU::addr_IZX, 8},
    {"BIT", &CPU::BIT, &CPU::addr_ZPG, 3}, {"AND", &CPU::AND, &CPU::addr_ZPG, 3},
    {"ROL", &CPU::ROL, &CPU::addr_ZPG, 5}, {"RLA", &CPU::RLA, &CPU::addr_ZPG, 5},
    {"PLP", &CPU::PLP, &CPU::addr_IMP, 4}, {"AND", &CPU::AND, &CPU::addr_IMM, 2},
    {"ROL", &CPU::ROL, &CPU::addr_ACC, 2}, {"???", &CPU::XXX, &CPU::addr_IMP, 2},
    {"BIT", &CPU::BIT, &CPU::addr_ABS, 4}, {"AND", &CPU::AND, &CPU::addr_ABS, 4},
    {"ROL", &CPU::ROL, &CPU::addr_ABS, 6}, {"RLA", &CPU::RLA, &CPU::addr_ABS, 6},
    
    // 0x30-0x3F
    {"BMI", &CPU::BMI, &CPU::addr_REL, 2}, {"AND", &CPU::AND, &CPU::addr_IZY, 5},
    {"???", &CPU::XXX, &CPU::addr_IMP, 2}, {"RLA", &CPU::RLA, &CPU::addr_IZY, 8},
    {"NOP", &CPU::NOP, &CPU::addr_ZPX, 4}, {"AND", &CPU::AND, &CPU::addr_ZPX, 4},
    {"ROL", &CPU::ROL, &CPU::addr_ZPX, 6}, {"RLA", &CPU::RLA, &CPU::addr_ZPX, 6},
    {"SEC", &CPU::SEC, &CPU::addr_IMP, 2}, {"AND", &CPU::AND, &CPU::addr_ABY, 4},
    {"NOP", &CPU::NOP, &CPU::addr_IMP, 2}, {"RLA", &CPU::RLA, &CPU::addr_ABY, 7},
    {"NOP", &CPU::NOP, &CPU::addr_ABX, 4}, {"AND", &CPU::AND, &CPU::addr_ABX, 4},
    {"ROL", &CPU::ROL, &CPU::addr_ABX, 7}, {"RLA", &CPU::RLA, &CPU::addr_ABX, 7},
    
    // 0x40-0x4F
    {"RTI", &CPU::RTI, &CPU::addr_IMP, 6}, {"EOR", &CPU::EOR, &CPU::addr_IZX, 6},
    {"???", &CPU::XXX, &CPU::addr_IMP, 2}, {"SRE", &CPU::SRE, &CPU::addr_IZX, 8},
    {"NOP", &CPU::NOP, &CPU::addr_ZPG, 3}, {"EOR", &CPU::EOR, &CPU::addr_ZPG, 3},
    {"LSR", &CPU::LSR, &CPU::addr_ZPG, 5}, {"SRE", &CPU::SRE, &CPU::addr_ZPG, 5},
    {"PHA", &CPU::PHA, &CPU::addr_IMP, 3}, {"EOR", &CPU::EOR, &CPU::addr_IMM, 2},
    {"LSR", &CPU::LSR, &CPU::addr_ACC, 2}, {"???", &CPU::XXX, &CPU::addr_IMP, 2},
    {"JMP", &CPU::JMP, &CPU::addr_ABS, 3}, {"EOR", &CPU::EOR, &CPU::addr_ABS, 4},
    {"LSR", &CPU::LSR, &CPU::addr_ABS, 6}, {"SRE", &CPU::SRE, &CPU::addr_ABS, 6},
    
    // 0x50-0x5F
    {"BVC", &CPU::BVC, &CPU::addr_REL, 2}, {"EOR", &CPU::EOR, &CPU::addr_IZY, 5},
    {"???", &CPU::XXX, &CPU::addr_IMP, 2}, {"SRE", &CPU::SRE, &CPU::addr_IZY, 8},
    {"NOP", &CPU::NOP, &CPU::addr_ZPX, 4}, {"EOR", &CPU::EOR, &CPU::addr_ZPX, 4},
    {"LSR", &CPU::LSR, &CPU::addr_ZPX, 6}, {"SRE", &CPU::SRE, &CPU::addr_ZPX, 6},
    {"CLI", &CPU::CLI, &CPU::addr_IMP, 2}, {"EOR", &CPU::EOR, &CPU::addr_ABY, 4},
    {"NOP", &CPU::NOP, &CPU::addr_IMP, 2}, {"SRE", &CPU::SRE, &CPU::addr_ABY, 7},
    {"NOP", &CPU::NOP, &CPU::addr_ABX, 4}, {"EOR", &CPU::EOR, &CPU::addr_ABX, 4},
    {"LSR", &CPU::LSR, &CPU::addr_ABX, 7}, {"SRE", &CPU::SRE, &CPU::addr_ABX, 7},
    
    // 0x60-0x6F
    {"RTS", &CPU::RTS, &CPU::addr_IMP, 6}, {"ADC", &CPU::ADC, &CPU::addr_IZX, 6},
    {"???", &CPU::XXX, &CPU::addr_IMP, 2}, {"RRA", &CPU::RRA, &CPU::addr_IZX, 8},
    {"NOP", &CPU::NOP, &CPU::addr_ZPG, 3}, {"ADC", &CPU::ADC, &CPU::addr_ZPG, 3},
    {"ROR", &CPU::ROR, &CPU::addr_ZPG, 5}, {"RRA", &CPU::RRA, &CPU::addr_ZPG, 5},
    {"PLA", &CPU::PLA, &CPU::addr_IMP, 4}, {"ADC", &CPU::ADC, &CPU::addr_IMM, 2},
    {"ROR", &CPU::ROR, &CPU::addr_ACC, 2}, {"???", &CPU::XXX, &CPU::addr_IMP, 2},
    {"JMP", &CPU::JMP, &CPU::addr_IND, 5}, {"ADC", &CPU::ADC, &CPU::addr_ABS, 4},
    {"ROR", &CPU::ROR, &CPU::addr_ABS, 6}, {"RRA", &CPU::RRA, &CPU::addr_ABS, 6},
    
    // 0x70-0x7F
    {"BVS", &CPU::BVS, &CPU::addr_REL, 2}, {"ADC", &CPU::ADC, &CPU::addr_IZY, 5},
    {"???", &CPU::XXX, &CPU::addr_IMP, 2}, {"RRA", &CPU::RRA, &CPU::addr_IZY, 8},
    {"NOP", &CPU::NOP, &CPU::addr_ZPX, 4}, {"ADC", &CPU::ADC, &CPU::addr_ZPX, 4},
    {"ROR", &CPU::ROR, &CPU::addr_ZPX, 6}, {"RRA", &CPU::RRA, &CPU::addr_ZPX, 6},
    {"SEI", &CPU::SEI, &CPU::addr_IMP, 2}, {"ADC", &CPU::ADC, &CPU::addr_ABY, 4},
    {"NOP", &CPU::NOP, &CPU::addr_IMP, 2}, {"RRA", &CPU::RRA, &CPU::addr_ABY, 7},
    {"NOP", &CPU::NOP, &CPU::addr_ABX, 4}, {"ADC", &CPU::ADC, &CPU::addr_ABX, 4},
    {"ROR", &CPU::ROR, &CPU::addr_ABX, 7}, {"RRA", &CPU::RRA, &CPU::addr_ABX, 7},
    
    // 0x80-0x8F
    {"NOP", &CPU::NOP, &CPU::addr_IMM, 2}, {"STA", &CPU::STA, &CPU::addr_IZX, 6},
    {"NOP", &CPU::NOP, &CPU::addr_IMM, 2}, {"SAX", &CPU::SAX, &CPU::addr_IZX, 6},
    {"STY", &CPU::STY, &CPU::addr_ZPG, 3}, {"STA", &CPU::STA, &CPU::addr_ZPG, 3},
    {"STX", &CPU::STX, &CPU::addr_ZPG, 3}, {"SAX", &CPU::SAX, &CPU::addr_ZPG, 3},
    {"DEY", &CPU::DEY, &CPU::addr_IMP, 2}, {"NOP", &CPU::NOP, &CPU::addr_IMM, 2},
    {"TXA", &CPU::TXA, &CPU::addr_IMP, 2}, {"???", &CPU::XXX, &CPU::addr_IMP, 2},
    {"STY", &CPU::STY, &CPU::addr_ABS, 4}, {"STA", &CPU::STA, &CPU::addr_ABS, 4},
    {"STX", &CPU::STX, &CPU::addr_ABS, 4}, {"SAX", &CPU::SAX, &CPU::addr_ABS, 4},
    
    // 0x90-0x9F
    {"BCC", &CPU::BCC, &CPU::addr_REL, 2}, {"STA", &CPU::STA, &CPU::addr_IZY, 6},
    {"???", &CPU::XXX, &CPU::addr_IMP, 2}, {"???", &CPU::XXX, &CPU::addr_IMP, 6},
    {"STY", &CPU::STY, &CPU::addr_ZPX, 4}, {"STA", &CPU::STA, &CPU::addr_ZPX, 4},
    {"STX", &CPU::STX, &CPU::addr_ZPY, 4}, {"SAX", &CPU::SAX, &CPU::addr_ZPY, 4},
    {"TYA", &CPU::TYA, &CPU::addr_IMP, 2}, {"STA", &CPU::STA, &CPU::addr_ABY, 5},
    {"TXS", &CPU::TXS, &CPU::addr_IMP, 2}, {"???", &CPU::XXX, &CPU::addr_IMP, 5},
    {"???", &CPU::XXX, &CPU::addr_IMP, 5}, {"STA", &CPU::STA, &CPU::addr_ABX, 5},
    {"???", &CPU::XXX, &CPU::addr_IMP, 5}, {"???", &CPU::XXX, &CPU::addr_IMP, 5},
    
    // 0xA0-0xAF
    {"LDY", &CPU::LDY, &CPU::addr_IMM, 2}, {"LDA", &CPU::LDA, &CPU::addr_IZX, 6},
    {"LDX", &CPU::LDX, &CPU::addr_IMM, 2}, {"LAX", &CPU::LAX, &CPU::addr_IZX, 6},
    {"LDY", &CPU::LDY, &CPU::addr_ZPG, 3}, {"LDA", &CPU::LDA, &CPU::addr_ZPG, 3},
    {"LDX", &CPU::LDX, &CPU::addr_ZPG, 3}, {"LAX", &CPU::LAX, &CPU::addr_ZPG, 3},
    {"TAY", &CPU::TAY, &CPU::addr_IMP, 2}, {"LDA", &CPU::LDA, &CPU::addr_IMM, 2},
    {"TAX", &CPU::TAX, &CPU::addr_IMP, 2}, {"???", &CPU::XXX, &CPU::addr_IMP, 2},
    {"LDY", &CPU::LDY, &CPU::addr_ABS, 4}, {"LDA", &CPU::LDA, &CPU::addr_ABS, 4},
    {"LDX", &CPU::LDX, &CPU::addr_ABS, 4}, {"LAX", &CPU::LAX, &CPU::addr_ABS, 4},
    
    // 0xB0-0xBF
    {"BCS", &CPU::BCS, &CPU::addr_REL, 2}, {"LDA", &CPU::LDA, &CPU::addr_IZY, 5},
    {"???", &CPU::XXX, &CPU::addr_IMP, 2}, {"LAX", &CPU::LAX, &CPU::addr_IZY, 5},
    {"LDY", &CPU::LDY, &CPU::addr_ZPX, 4}, {"LDA", &CPU::LDA, &CPU::addr_ZPX, 4},
    {"LDX", &CPU::LDX, &CPU::addr_ZPY, 4}, {"LAX", &CPU::LAX, &CPU::addr_ZPY, 4},
    {"CLV", &CPU::CLV, &CPU::addr_IMP, 2}, {"LDA", &CPU::LDA, &CPU::addr_ABY, 4},
    {"TSX", &CPU::TSX, &CPU::addr_IMP, 2}, {"???", &CPU::XXX, &CPU::addr_IMP, 4},
    {"LDY", &CPU::LDY, &CPU::addr_ABX, 4}, {"LDA", &CPU::LDA, &CPU::addr_ABX, 4},
    {"LDX", &CPU::LDX, &CPU::addr_ABY, 4}, {"LAX", &CPU::LAX, &CPU::addr_ABY, 4},
    
    // 0xC0-0xCF
    {"CPY", &CPU::CPY, &CPU::addr_IMM, 2}, {"CMP", &CPU::CMP, &CPU::addr_IZX, 6},
    {"NOP", &CPU::NOP, &CPU::addr_IMM, 2}, {"DCP", &CPU::DCP, &CPU::addr_IZX, 8},
    {"CPY", &CPU::CPY, &CPU::addr_ZPG, 3}, {"CMP", &CPU::CMP, &CPU::addr_ZPG, 3},
    {"DEC", &CPU::DEC, &CPU::addr_ZPG, 5}, {"DCP", &CPU::DCP, &CPU::addr_ZPG, 5},
    {"INY", &CPU::INY, &CPU::addr_IMP, 2}, {"CMP", &CPU::CMP, &CPU::addr_IMM, 2},
    {"DEX", &CPU::DEX, &CPU::addr_IMP, 2}, {"???", &CPU::XXX, &CPU::addr_IMP, 2},
    {"CPY", &CPU::CPY, &CPU::addr_ABS, 4}, {"CMP", &CPU::CMP, &CPU::addr_ABS, 4},
    {"DEC", &CPU::DEC, &CPU::addr_ABS, 6}, {"DCP", &CPU::DCP, &CPU::addr_ABS, 6},
    
    // 0xD0-0xDF
    {"BNE", &CPU::BNE, &CPU::addr_REL, 2}, {"CMP", &CPU::CMP, &CPU::addr_IZY, 5},
    {"???", &CPU::XXX, &CPU::addr_IMP, 2}, {"DCP", &CPU::DCP, &CPU::addr_IZY, 8},
    {"NOP", &CPU::NOP, &CPU::addr_ZPX, 4}, {"CMP", &CPU::CMP, &CPU::addr_ZPX, 4},
    {"DEC", &CPU::DEC, &CPU::addr_ZPX, 6}, {"DCP", &CPU::DCP, &CPU::addr_ZPX, 6},
    {"CLD", &CPU::CLD, &CPU::addr_IMP, 2}, {"CMP", &CPU::CMP, &CPU::addr_ABY, 4},
    {"NOP", &CPU::NOP, &CPU::addr_IMP, 2}, {"DCP", &CPU::DCP, &CPU::addr_ABY, 7},
    {"NOP", &CPU::NOP, &CPU::addr_ABX, 4}, {"CMP", &CPU::CMP, &CPU::addr_ABX, 4},
    {"DEC", &CPU::DEC, &CPU::addr_ABX, 7}, {"DCP", &CPU::DCP, &CPU::addr_ABX, 7},
    
    // 0xE0-0xEF
    {"CPX", &CPU::CPX, &CPU::addr_IMM, 2}, {"SBC", &CPU::SBC, &CPU::addr_IZX, 6},
    {"NOP", &CPU::NOP, &CPU::addr_IMM, 2}, {"ISB", &CPU::ISB, &CPU::addr_IZX, 8},
    {"CPX", &CPU::CPX, &CPU::addr_ZPG, 3}, {"SBC", &CPU::SBC, &CPU::addr_ZPG, 3},
    {"INC", &CPU::INC, &CPU::addr_ZPG, 5}, {"ISB", &CPU::ISB, &CPU::addr_ZPG, 5},
    {"INX", &CPU::INX, &CPU::addr_IMP, 2}, {"SBC", &CPU::SBC, &CPU::addr_IMM, 2},
    {"NOP", &CPU::NOP, &CPU::addr_IMP, 2}, {"SBC", &CPU::SBC, &CPU::addr_IMM, 2},
    {"CPX", &CPU::CPX, &CPU::addr_ABS, 4}, {"SBC", &CPU::SBC, &CPU::addr_ABS, 4},
    {"INC", &CPU::INC, &CPU::addr_ABS, 6}, {"ISB", &CPU::ISB, &CPU::addr_ABS, 6},
    
    // 0xF0-0xFF
    {"BEQ", &CPU::BEQ, &CPU::addr_REL, 2}, {"SBC", &CPU::SBC, &CPU::addr_IZY, 5},
    {"???", &CPU::XXX, &CPU::addr_IMP, 2}, {"ISB", &CPU::ISB, &CPU::addr_IZY, 8},
    {"NOP", &CPU::NOP, &CPU::addr_ZPX, 4}, {"SBC", &CPU::SBC, &CPU::addr_ZPX, 4},
    {"INC", &CPU::INC, &CPU::addr_ZPX, 6}, {"ISB", &CPU::ISB, &CPU::addr_ZPX, 6},
    {"SED", &CPU::SED, &CPU::addr_IMP, 2}, {"SBC", &CPU::SBC, &CPU::addr_ABY, 4},
    {"NOP", &CPU::NOP, &CPU::addr_IMP, 2}, {"ISB", &CPU::ISB, &CPU::addr_ABY, 7},
    {"NOP", &CPU::NOP, &CPU::addr_ABX, 4}, {"SBC", &CPU::SBC, &CPU::addr_ABX, 4},
    {"INC", &CPU::INC, &CPU::addr_ABX, 7}, {"ISB", &CPU::ISB, &CPU::addr_ABX, 7},
}};

// =============================================================================
// Constructor and Core
// =============================================================================

CPU::CPU(Memory& bus) : bus_(bus) {}

void CPU::reset() {
    // Read reset vector
    std::uint16_t lo = read(0xFFFC);
    std::uint16_t hi = read(0xFFFD);
    pc_ = (hi << 8) | lo;
    
    // Reset registers
    a_ = 0;
    x_ = 0;
    y_ = 0;
    sp_ = 0xFD;  // Stack starts at 0x01FD
    status_ = 0x24;  // Unused flag always set, IRQ disabled
    
    // Reset state
    addr_abs_ = 0;
    addr_rel_ = 0;
    fetched_ = 0;
    extra_cycles_ = 0;
    
    // Reset takes 8 cycles
    cycles_ = 8;
}

int CPU::step() {
    // Fetch opcode
    opcode_ = read(pc_++);
    
    // Always set unused flag
    set_flag(Flag::U, true);
    
    // Get instruction info
    const Instruction& inst = INSTRUCTION_TABLE[opcode_];
    
    // Execute addressing mode
    std::uint8_t addr_cycles = (this->*inst.addrmode)();
    
    // Execute instruction
    std::uint8_t op_cycles = (this->*inst.operate)();
    
    // Calculate total cycles
    std::uint8_t total_cycles = inst.cycles + (addr_cycles & op_cycles);
    
    // Always set unused flag
    set_flag(Flag::U, true);
    
    cycles_ += total_cycles;
    
    return total_cycles;
}

// =============================================================================
// Memory Access
// =============================================================================

std::uint8_t CPU::read(std::uint16_t addr) {
    return bus_.read(addr);
}

void CPU::write(std::uint16_t addr, std::uint8_t data) {
    bus_.write(addr, data);
}

// =============================================================================
// Stack Operations
// =============================================================================

void CPU::push(std::uint8_t data) {
    write(0x0100 + sp_, data);
    sp_--;
}

void CPU::push16(std::uint16_t data) {
    push((data >> 8) & 0xFF);
    push(data & 0xFF);
}

std::uint8_t CPU::pop() {
    sp_++;
    return read(0x0100 + sp_);
}

std::uint16_t CPU::pop16() {
    std::uint16_t lo = pop();
    std::uint16_t hi = pop();
    return (hi << 8) | lo;
}

// =============================================================================
// Flag Operations
// =============================================================================

bool CPU::get_flag(Flag f) const {
    return (status_ & (1 << static_cast<std::uint8_t>(f))) != 0;
}

void CPU::set_flag(Flag f, bool v) {
    if (v) {
        status_ |= (1 << static_cast<std::uint8_t>(f));
    } else {
        status_ &= ~(1 << static_cast<std::uint8_t>(f));
    }
}

void CPU::set_nz(std::uint8_t value) {
    set_flag(Flag::Z, value == 0);
    set_flag(Flag::N, (value & 0x80) != 0);
}

// =============================================================================
// Addressing Modes
// =============================================================================

std::uint8_t CPU::addr_IMP() {
    fetched_ = a_;
    return 0;
}

std::uint8_t CPU::addr_ACC() {
    fetched_ = a_;
    return 0;
}

std::uint8_t CPU::addr_IMM() {
    addr_abs_ = pc_++;
    return 0;
}

std::uint8_t CPU::addr_ZPG() {
    addr_abs_ = read(pc_++);
    addr_abs_ &= 0x00FF;
    return 0;
}

std::uint8_t CPU::addr_ZPX() {
    addr_abs_ = (read(pc_++) + x_) & 0x00FF;
    return 0;
}

std::uint8_t CPU::addr_ZPY() {
    addr_abs_ = (read(pc_++) + y_) & 0x00FF;
    return 0;
}

std::uint8_t CPU::addr_REL() {
    addr_rel_ = read(pc_++);
    if (addr_rel_ & 0x80) {
        addr_rel_ |= 0xFF00;  // Sign extend
    }
    return 0;
}

std::uint8_t CPU::addr_ABS() {
    std::uint16_t lo = read(pc_++);
    std::uint16_t hi = read(pc_++);
    addr_abs_ = (hi << 8) | lo;
    return 0;
}

std::uint8_t CPU::addr_ABX() {
    std::uint16_t lo = read(pc_++);
    std::uint16_t hi = read(pc_++);
    addr_abs_ = ((hi << 8) | lo) + x_;
    
    // Page boundary crossed?
    if ((addr_abs_ & 0xFF00) != (hi << 8)) {
        return 1;
    }
    return 0;
}

std::uint8_t CPU::addr_ABY() {
    std::uint16_t lo = read(pc_++);
    std::uint16_t hi = read(pc_++);
    addr_abs_ = ((hi << 8) | lo) + y_;
    
    if ((addr_abs_ & 0xFF00) != (hi << 8)) {
        return 1;
    }
    return 0;
}

std::uint8_t CPU::addr_IND() {
    std::uint16_t ptr_lo = read(pc_++);
    std::uint16_t ptr_hi = read(pc_++);
    std::uint16_t ptr = (ptr_hi << 8) | ptr_lo;
    
    // 6502 bug: page boundary wraparound
    if (ptr_lo == 0x00FF) {
        addr_abs_ = (read(ptr & 0xFF00) << 8) | read(ptr);
    } else {
        addr_abs_ = (read(ptr + 1) << 8) | read(ptr);
    }
    return 0;
}

std::uint8_t CPU::addr_IZX() {
    std::uint16_t t = read(pc_++);
    std::uint16_t lo = read((t + x_) & 0x00FF);
    std::uint16_t hi = read((t + x_ + 1) & 0x00FF);
    addr_abs_ = (hi << 8) | lo;
    return 0;
}

std::uint8_t CPU::addr_IZY() {
    std::uint16_t t = read(pc_++);
    std::uint16_t lo = read(t & 0x00FF);
    std::uint16_t hi = read((t + 1) & 0x00FF);
    addr_abs_ = ((hi << 8) | lo) + y_;
    
    if ((addr_abs_ & 0xFF00) != (hi << 8)) {
        return 1;
    }
    return 0;
}

std::uint8_t CPU::fetch() {
    const Instruction& inst = INSTRUCTION_TABLE[opcode_];
    if (inst.addrmode != &CPU::addr_IMP && inst.addrmode != &CPU::addr_ACC) {
        fetched_ = read(addr_abs_);
    }
    return fetched_;
}

// =============================================================================
// Interrupts
// =============================================================================

void CPU::nmi() {
    push16(pc_);
    
    set_flag(Flag::B, false);
    set_flag(Flag::U, true);
    set_flag(Flag::I, true);
    push(status_);
    
    std::uint16_t lo = read(0xFFFA);
    std::uint16_t hi = read(0xFFFB);
    pc_ = (hi << 8) | lo;
    
    cycles_ += 8;
}

void CPU::irq() {
    if (!get_flag(Flag::I)) {
        push16(pc_);
        
        set_flag(Flag::B, false);
        set_flag(Flag::U, true);
        set_flag(Flag::I, true);
        push(status_);
        
        std::uint16_t lo = read(0xFFFE);
        std::uint16_t hi = read(0xFFFF);
        pc_ = (hi << 8) | lo;
        
        cycles_ += 7;
    }
}

// =============================================================================
// Branch Helper
// =============================================================================

void CPU::branch_if(bool condition) {
    if (condition) {
        cycles_++;
        addr_abs_ = pc_ + addr_rel_;
        
        // Extra cycle if page boundary crossed
        if ((addr_abs_ & 0xFF00) != (pc_ & 0xFF00)) {
            cycles_++;
        }
        
        pc_ = addr_abs_;
    }
}

// =============================================================================
// Instructions - Load/Store
// =============================================================================

std::uint8_t CPU::LDA() {
    a_ = fetch();
    set_nz(a_);
    return 1;
}

std::uint8_t CPU::LDX() {
    x_ = fetch();
    set_nz(x_);
    return 1;
}

std::uint8_t CPU::LDY() {
    y_ = fetch();
    set_nz(y_);
    return 1;
}

std::uint8_t CPU::STA() {
    write(addr_abs_, a_);
    return 0;
}

std::uint8_t CPU::STX() {
    write(addr_abs_, x_);
    return 0;
}

std::uint8_t CPU::STY() {
    write(addr_abs_, y_);
    return 0;
}

// =============================================================================
// Instructions - Transfer
// =============================================================================

std::uint8_t CPU::TAX() {
    x_ = a_;
    set_nz(x_);
    return 0;
}

std::uint8_t CPU::TAY() {
    y_ = a_;
    set_nz(y_);
    return 0;
}

std::uint8_t CPU::TXA() {
    a_ = x_;
    set_nz(a_);
    return 0;
}

std::uint8_t CPU::TYA() {
    a_ = y_;
    set_nz(a_);
    return 0;
}

std::uint8_t CPU::TSX() {
    x_ = sp_;
    set_nz(x_);
    return 0;
}

std::uint8_t CPU::TXS() {
    sp_ = x_;
    return 0;
}

// =============================================================================
// Instructions - Stack
// =============================================================================

std::uint8_t CPU::PHA() {
    push(a_);
    return 0;
}

std::uint8_t CPU::PHP() {
    push(status_ | (1 << static_cast<std::uint8_t>(Flag::B)) | 
                   (1 << static_cast<std::uint8_t>(Flag::U)));
    return 0;
}

std::uint8_t CPU::PLA() {
    a_ = pop();
    set_nz(a_);
    return 0;
}

std::uint8_t CPU::PLP() {
    status_ = pop();
    set_flag(Flag::U, true);
    set_flag(Flag::B, false);
    return 0;
}

// =============================================================================
// Instructions - Arithmetic
// =============================================================================

std::uint8_t CPU::ADC() {
    fetch();
    std::uint16_t temp = static_cast<std::uint16_t>(a_) + 
                         static_cast<std::uint16_t>(fetched_) + 
                         (get_flag(Flag::C) ? 1 : 0);
    
    set_flag(Flag::C, temp > 255);
    set_flag(Flag::Z, (temp & 0x00FF) == 0);
    set_flag(Flag::V, (~(a_ ^ fetched_) & (a_ ^ temp)) & 0x0080);
    set_flag(Flag::N, temp & 0x80);
    
    a_ = temp & 0x00FF;
    return 1;
}

std::uint8_t CPU::SBC() {
    fetch();
    std::uint16_t value = static_cast<std::uint16_t>(fetched_) ^ 0x00FF;
    std::uint16_t temp = static_cast<std::uint16_t>(a_) + value + 
                         (get_flag(Flag::C) ? 1 : 0);
    
    set_flag(Flag::C, temp & 0xFF00);
    set_flag(Flag::Z, (temp & 0x00FF) == 0);
    set_flag(Flag::V, (temp ^ a_) & (temp ^ value) & 0x0080);
    set_flag(Flag::N, temp & 0x0080);
    
    a_ = temp & 0x00FF;
    return 1;
}

std::uint8_t CPU::INC() {
    fetch();
    std::uint8_t temp = fetched_ + 1;
    write(addr_abs_, temp);
    set_nz(temp);
    return 0;
}

std::uint8_t CPU::INX() {
    x_++;
    set_nz(x_);
    return 0;
}

std::uint8_t CPU::INY() {
    y_++;
    set_nz(y_);
    return 0;
}

std::uint8_t CPU::DEC() {
    fetch();
    std::uint8_t temp = fetched_ - 1;
    write(addr_abs_, temp);
    set_nz(temp);
    return 0;
}

std::uint8_t CPU::DEX() {
    x_--;
    set_nz(x_);
    return 0;
}

std::uint8_t CPU::DEY() {
    y_--;
    set_nz(y_);
    return 0;
}

// =============================================================================
// Instructions - Logical
// =============================================================================

std::uint8_t CPU::AND() {
    a_ &= fetch();
    set_nz(a_);
    return 1;
}

std::uint8_t CPU::ORA() {
    a_ |= fetch();
    set_nz(a_);
    return 1;
}

std::uint8_t CPU::EOR() {
    a_ ^= fetch();
    set_nz(a_);
    return 1;
}

// =============================================================================
// Instructions - Shift/Rotate
// =============================================================================

std::uint8_t CPU::ASL() {
    fetch();
    std::uint16_t temp = static_cast<std::uint16_t>(fetched_) << 1;
    set_flag(Flag::C, (temp & 0xFF00) > 0);
    set_nz(temp & 0x00FF);
    
    if (INSTRUCTION_TABLE[opcode_].addrmode == &CPU::addr_ACC) {
        a_ = temp & 0x00FF;
    } else {
        write(addr_abs_, temp & 0x00FF);
    }
    return 0;
}

std::uint8_t CPU::LSR() {
    fetch();
    set_flag(Flag::C, fetched_ & 0x01);
    std::uint8_t temp = fetched_ >> 1;
    set_nz(temp);
    
    if (INSTRUCTION_TABLE[opcode_].addrmode == &CPU::addr_ACC) {
        a_ = temp;
    } else {
        write(addr_abs_, temp);
    }
    return 0;
}

std::uint8_t CPU::ROL() {
    fetch();
    std::uint16_t temp = (static_cast<std::uint16_t>(fetched_) << 1) | 
                         (get_flag(Flag::C) ? 1 : 0);
    set_flag(Flag::C, temp & 0xFF00);
    set_nz(temp & 0x00FF);
    
    if (INSTRUCTION_TABLE[opcode_].addrmode == &CPU::addr_ACC) {
        a_ = temp & 0x00FF;
    } else {
        write(addr_abs_, temp & 0x00FF);
    }
    return 0;
}

std::uint8_t CPU::ROR() {
    fetch();
    std::uint16_t temp = (get_flag(Flag::C) ? 0x80 : 0x00) | (fetched_ >> 1);
    set_flag(Flag::C, fetched_ & 0x01);
    set_nz(temp & 0x00FF);
    
    if (INSTRUCTION_TABLE[opcode_].addrmode == &CPU::addr_ACC) {
        a_ = temp & 0x00FF;
    } else {
        write(addr_abs_, temp & 0x00FF);
    }
    return 0;
}

// =============================================================================
// Instructions - Compare
// =============================================================================

std::uint8_t CPU::CMP() {
    fetch();
    std::uint16_t temp = static_cast<std::uint16_t>(a_) - static_cast<std::uint16_t>(fetched_);
    set_flag(Flag::C, a_ >= fetched_);
    set_nz(temp & 0x00FF);
    return 1;
}

std::uint8_t CPU::CPX() {
    fetch();
    std::uint16_t temp = static_cast<std::uint16_t>(x_) - static_cast<std::uint16_t>(fetched_);
    set_flag(Flag::C, x_ >= fetched_);
    set_nz(temp & 0x00FF);
    return 0;
}

std::uint8_t CPU::CPY() {
    fetch();
    std::uint16_t temp = static_cast<std::uint16_t>(y_) - static_cast<std::uint16_t>(fetched_);
    set_flag(Flag::C, y_ >= fetched_);
    set_nz(temp & 0x00FF);
    return 0;
}

// =============================================================================
// Instructions - Branch
// =============================================================================

std::uint8_t CPU::BCC() { branch_if(!get_flag(Flag::C)); return 0; }
std::uint8_t CPU::BCS() { branch_if(get_flag(Flag::C)); return 0; }
std::uint8_t CPU::BEQ() { branch_if(get_flag(Flag::Z)); return 0; }
std::uint8_t CPU::BNE() { branch_if(!get_flag(Flag::Z)); return 0; }
std::uint8_t CPU::BMI() { branch_if(get_flag(Flag::N)); return 0; }
std::uint8_t CPU::BPL() { branch_if(!get_flag(Flag::N)); return 0; }
std::uint8_t CPU::BVC() { branch_if(!get_flag(Flag::V)); return 0; }
std::uint8_t CPU::BVS() { branch_if(get_flag(Flag::V)); return 0; }

// =============================================================================
// Instructions - Jump/Call
// =============================================================================

std::uint8_t CPU::JMP() {
    pc_ = addr_abs_;
    return 0;
}

std::uint8_t CPU::JSR() {
    push16(pc_ - 1);
    pc_ = addr_abs_;
    return 0;
}

std::uint8_t CPU::RTS() {
    pc_ = pop16() + 1;
    return 0;
}

std::uint8_t CPU::RTI() {
    status_ = pop();
    set_flag(Flag::B, false);
    set_flag(Flag::U, true);
    pc_ = pop16();
    return 0;
}

// =============================================================================
// Instructions - Flag Operations
// =============================================================================

std::uint8_t CPU::CLC() { set_flag(Flag::C, false); return 0; }
std::uint8_t CPU::CLD() { set_flag(Flag::D, false); return 0; }
std::uint8_t CPU::CLI() { set_flag(Flag::I, false); return 0; }
std::uint8_t CPU::CLV() { set_flag(Flag::V, false); return 0; }
std::uint8_t CPU::SEC() { set_flag(Flag::C, true); return 0; }
std::uint8_t CPU::SED() { set_flag(Flag::D, true); return 0; }
std::uint8_t CPU::SEI() { set_flag(Flag::I, true); return 0; }

// =============================================================================
// Instructions - Misc
// =============================================================================

std::uint8_t CPU::BIT() {
    fetch();
    set_flag(Flag::Z, (a_ & fetched_) == 0);
    set_flag(Flag::N, fetched_ & 0x80);
    set_flag(Flag::V, fetched_ & 0x40);
    return 0;
}

std::uint8_t CPU::NOP() {
    // Some NOPs have different cycle counts based on addressing mode
    return 0;
}

std::uint8_t CPU::BRK() {
    pc_++;
    
    set_flag(Flag::I, true);
    push16(pc_);
    
    set_flag(Flag::B, true);
    push(status_);
    set_flag(Flag::B, false);
    
    std::uint16_t lo = read(0xFFFE);
    std::uint16_t hi = read(0xFFFF);
    pc_ = (hi << 8) | lo;
    
    return 0;
}

// =============================================================================
// Illegal/Unofficial Opcodes
// =============================================================================

std::uint8_t CPU::XXX() {
    // Unimplemented opcode - NOP equivalent
    return 0;
}

std::uint8_t CPU::LAX() {
    // LDA + LDX
    a_ = fetch();
    x_ = a_;
    set_nz(a_);
    return 1;
}

std::uint8_t CPU::SAX() {
    // Store A & X
    write(addr_abs_, a_ & x_);
    return 0;
}

std::uint8_t CPU::DCP() {
    // DEC + CMP
    fetch();
    std::uint8_t temp = fetched_ - 1;
    write(addr_abs_, temp);
    
    std::uint16_t cmp = static_cast<std::uint16_t>(a_) - static_cast<std::uint16_t>(temp);
    set_flag(Flag::C, a_ >= temp);
    set_nz(cmp & 0x00FF);
    return 0;
}

std::uint8_t CPU::ISB() {
    // INC + SBC
    fetch();
    std::uint8_t temp = fetched_ + 1;
    write(addr_abs_, temp);
    
    std::uint16_t value = static_cast<std::uint16_t>(temp) ^ 0x00FF;
    std::uint16_t result = static_cast<std::uint16_t>(a_) + value + 
                           (get_flag(Flag::C) ? 1 : 0);
    
    set_flag(Flag::C, result & 0xFF00);
    set_flag(Flag::Z, (result & 0x00FF) == 0);
    set_flag(Flag::V, (result ^ a_) & (result ^ value) & 0x0080);
    set_flag(Flag::N, result & 0x0080);
    
    a_ = result & 0x00FF;
    return 0;
}

std::uint8_t CPU::SLO() {
    // ASL + ORA
    fetch();
    std::uint16_t temp = static_cast<std::uint16_t>(fetched_) << 1;
    set_flag(Flag::C, (temp & 0xFF00) > 0);
    write(addr_abs_, temp & 0x00FF);
    
    a_ |= (temp & 0x00FF);
    set_nz(a_);
    return 0;
}

std::uint8_t CPU::RLA() {
    // ROL + AND
    fetch();
    std::uint16_t temp = (static_cast<std::uint16_t>(fetched_) << 1) | 
                         (get_flag(Flag::C) ? 1 : 0);
    set_flag(Flag::C, temp & 0xFF00);
    write(addr_abs_, temp & 0x00FF);
    
    a_ &= (temp & 0x00FF);
    set_nz(a_);
    return 0;
}

std::uint8_t CPU::SRE() {
    // LSR + EOR
    fetch();
    set_flag(Flag::C, fetched_ & 0x01);
    std::uint8_t temp = fetched_ >> 1;
    write(addr_abs_, temp);
    
    a_ ^= temp;
    set_nz(a_);
    return 0;
}

std::uint8_t CPU::RRA() {
    // ROR + ADC
    fetch();
    std::uint8_t temp = (get_flag(Flag::C) ? 0x80 : 0x00) | (fetched_ >> 1);
    set_flag(Flag::C, fetched_ & 0x01);
    write(addr_abs_, temp);
    
    std::uint16_t result = static_cast<std::uint16_t>(a_) + 
                           static_cast<std::uint16_t>(temp) + 
                           (get_flag(Flag::C) ? 1 : 0);
    
    set_flag(Flag::C, result > 255);
    set_flag(Flag::Z, (result & 0x00FF) == 0);
    set_flag(Flag::V, (~(a_ ^ temp) & (a_ ^ result)) & 0x0080);
    set_flag(Flag::N, result & 0x80);
    
    a_ = result & 0x00FF;
    return 0;
}
