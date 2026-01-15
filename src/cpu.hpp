#pragma once

#include <cstdint>
#include <array>
#include <functional>

class Memory;

// 6502 Status Flags
enum class Flag : std::uint8_t {
    C = 0,  // Carry
    Z = 1,  // Zero
    I = 2,  // Interrupt Disable
    D = 3,  // Decimal Mode (not used in NES)
    B = 4,  // Break Command
    U = 5,  // Unused (always 1)
    V = 6,  // Overflow
    N = 7   // Negative
};

// Addressing Modes
enum class AddressMode {
    IMP,    // Implied
    ACC,    // Accumulator
    IMM,    // Immediate
    ZPG,    // Zero Page
    ZPX,    // Zero Page,X
    ZPY,    // Zero Page,Y
    REL,    // Relative
    ABS,    // Absolute
    ABX,    // Absolute,X
    ABY,    // Absolute,Y
    IND,    // Indirect
    IZX,    // Indexed Indirect (X)
    IZY     // Indirect Indexed (Y)
};

class CPU {
public:
    explicit CPU(Memory& bus);

    // Main interface
    void reset();
    int step();  // Execute one instruction, return cycles consumed
    
    // Interrupts
    void nmi();   // Non-Maskable Interrupt
    void irq();   // Interrupt Request
    
    // State accessors (for debugging/testing)
    std::uint8_t get_a() const { return a_; }
    std::uint8_t get_x() const { return x_; }
    std::uint8_t get_y() const { return y_; }
    std::uint8_t get_sp() const { return sp_; }
    std::uint16_t get_pc() const { return pc_; }
    std::uint8_t get_status() const { return status_; }
    std::uint64_t get_cycles() const { return cycles_; }
    
    // Debug info for current instruction
    std::uint8_t get_opcode() const { return opcode_; }
    const char* get_instruction_name() const { 
        return INSTRUCTION_TABLE[opcode_].name; 
    }
    std::uint16_t get_addr_abs() const { return addr_abs_; }
    std::uint8_t get_fetched() const { return fetched_; }

    // State mutators (for testing)
    void set_a(std::uint8_t v) { a_ = v; }
    void set_x(std::uint8_t v) { x_ = v; }
    void set_y(std::uint8_t v) { y_ = v; }
    void set_sp(std::uint8_t v) { sp_ = v; }
    void set_pc(std::uint16_t v) { pc_ = v; }
    void set_status(std::uint8_t v) { status_ = v; }
    
    // Flag accessors - inline for performance
    [[nodiscard]] bool get_flag(Flag f) const {
        return (status_ & (1 << static_cast<std::uint8_t>(f))) != 0;
    }
    void set_flag(Flag f, bool v) {
        if (v) {
            status_ |= (1 << static_cast<std::uint8_t>(f));
        } else {
            status_ &= ~(1 << static_cast<std::uint8_t>(f));
        }
    }

private:
    Memory& bus_;
    
    // Registers
    std::uint8_t a_ = 0;       // Accumulator
    std::uint8_t x_ = 0;       // X Index Register
    std::uint8_t y_ = 0;       // Y Index Register
    std::uint8_t sp_ = 0;      // Stack Pointer
    std::uint16_t pc_ = 0;     // Program Counter
    std::uint8_t status_ = 0;  // Status Register (flags)
    
    // Cycle counting
    std::uint64_t cycles_ = 0;
    std::uint8_t extra_cycles_ = 0;  // Page boundary crossing cycles
    
    // Current instruction state
    std::uint16_t addr_abs_ = 0;     // Absolute address
    std::uint16_t addr_rel_ = 0;     // Relative address offset
    std::uint8_t opcode_ = 0;        // Current opcode
    std::uint8_t fetched_ = 0;       // Fetched data
    
    // Memory access
    std::uint8_t read(std::uint16_t addr);
    void write(std::uint16_t addr, std::uint8_t data);
    
    // Stack operations
    void push(std::uint8_t data);
    void push16(std::uint16_t data);
    std::uint8_t pop();
    std::uint16_t pop16();
    
    // Fetch data based on addressing mode
    std::uint8_t fetch();
    
    // Addressing mode handlers (return extra cycles if page crossed)
    std::uint8_t addr_IMP();
    std::uint8_t addr_ACC();
    std::uint8_t addr_IMM();
    std::uint8_t addr_ZPG();
    std::uint8_t addr_ZPX();
    std::uint8_t addr_ZPY();
    std::uint8_t addr_REL();
    std::uint8_t addr_ABS();
    std::uint8_t addr_ABX();
    std::uint8_t addr_ABY();
    std::uint8_t addr_IND();
    std::uint8_t addr_IZX();
    std::uint8_t addr_IZY();
    
    // Instruction implementations (return extra cycles)
    // Load/Store
    std::uint8_t LDA(); std::uint8_t LDX(); std::uint8_t LDY();
    std::uint8_t STA(); std::uint8_t STX(); std::uint8_t STY();
    
    // Transfer
    std::uint8_t TAX(); std::uint8_t TAY(); std::uint8_t TXA();
    std::uint8_t TYA(); std::uint8_t TSX(); std::uint8_t TXS();
    
    // Stack
    std::uint8_t PHA(); std::uint8_t PHP(); std::uint8_t PLA(); std::uint8_t PLP();
    
    // Arithmetic
    std::uint8_t ADC(); std::uint8_t SBC();
    std::uint8_t INC(); std::uint8_t INX(); std::uint8_t INY();
    std::uint8_t DEC(); std::uint8_t DEX(); std::uint8_t DEY();
    
    // Logical
    std::uint8_t AND(); std::uint8_t ORA(); std::uint8_t EOR();
    
    // Shift/Rotate
    std::uint8_t ASL(); std::uint8_t LSR(); std::uint8_t ROL(); std::uint8_t ROR();
    
    // Compare
    std::uint8_t CMP(); std::uint8_t CPX(); std::uint8_t CPY();
    
    // Branch
    std::uint8_t BCC(); std::uint8_t BCS(); std::uint8_t BEQ(); std::uint8_t BNE();
    std::uint8_t BMI(); std::uint8_t BPL(); std::uint8_t BVC(); std::uint8_t BVS();
    
    // Jump/Call
    std::uint8_t JMP(); std::uint8_t JSR(); std::uint8_t RTS(); std::uint8_t RTI();
    
    // Flag operations
    std::uint8_t CLC(); std::uint8_t CLD(); std::uint8_t CLI(); std::uint8_t CLV();
    std::uint8_t SEC(); std::uint8_t SED(); std::uint8_t SEI();
    
    // Misc
    std::uint8_t BIT(); std::uint8_t NOP(); std::uint8_t BRK();
    
    // Illegal/Unofficial (common ones for compatibility)
    std::uint8_t XXX();  // Catch-all for unimplemented opcodes
    std::uint8_t LAX(); std::uint8_t SAX(); std::uint8_t DCP(); std::uint8_t ISB();
    std::uint8_t SLO(); std::uint8_t RLA(); std::uint8_t SRE(); std::uint8_t RRA();
    
    // Instruction table entry
    struct Instruction {
        const char* name;                           // Mnemonic
        std::uint8_t (CPU::*operate)();             // Operation function
        std::uint8_t (CPU::*addrmode)();            // Addressing mode function
        std::uint8_t cycles;                         // Base cycle count
    };
    
    // Instruction lookup table (256 entries)
    static const std::array<Instruction, 256> INSTRUCTION_TABLE;
    
    // Helper for setting N and Z flags
    void set_nz(std::uint8_t value);
    
    // Branch helper
    void branch_if(bool condition);
};
