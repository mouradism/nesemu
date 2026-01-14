#include <gtest/gtest.h>
#include "cpu.hpp"
#include "memory.hpp"
#include "cartridge.hpp"

// =============================================================================
// CPU Test Fixture
// =============================================================================

class CPUTest : public ::testing::Test {
protected:
    Cartridge cart{""};
    Memory memory{cart};
    CPU cpu{memory};
    
    void SetUp() override {
        // Initialize RAM to 0
        for (int i = 0; i < 2048; ++i) {
            memory.write(static_cast<std::uint16_t>(i), 0);
        }
        
        // Set up a simple reset vector pointing to $0200
        // We can't write to ROM, so we'll manually set PC
        cpu.set_pc(0x0200);
        cpu.set_sp(0xFD);
        cpu.set_status(0x24);  // Unused and IRQ disable set
        cpu.set_a(0);
        cpu.set_x(0);
        cpu.set_y(0);
    }
    
    // Helper to load program into RAM at $0200
    void load_program(const std::vector<std::uint8_t>& program) {
        for (std::size_t i = 0; i < program.size(); ++i) {
            memory.write(0x0200 + static_cast<std::uint16_t>(i), program[i]);
        }
    }
};

// =============================================================================
// Register Tests
// =============================================================================

TEST_F(CPUTest, InitialState) {
    EXPECT_EQ(cpu.get_pc(), 0x0200);
    EXPECT_EQ(cpu.get_sp(), 0xFD);
    EXPECT_EQ(cpu.get_a(), 0);
    EXPECT_EQ(cpu.get_x(), 0);
    EXPECT_EQ(cpu.get_y(), 0);
}

TEST_F(CPUTest, SetGetRegisters) {
    cpu.set_a(0x42);
    cpu.set_x(0x55);
    cpu.set_y(0xAA);
    cpu.set_sp(0xFF);
    cpu.set_pc(0x8000);
    
    EXPECT_EQ(cpu.get_a(), 0x42);
    EXPECT_EQ(cpu.get_x(), 0x55);
    EXPECT_EQ(cpu.get_y(), 0xAA);
    EXPECT_EQ(cpu.get_sp(), 0xFF);
    EXPECT_EQ(cpu.get_pc(), 0x8000);
}

// =============================================================================
// Flag Tests
// =============================================================================

TEST_F(CPUTest, FlagOperations) {
    cpu.set_flag(Flag::C, true);
    EXPECT_TRUE(cpu.get_flag(Flag::C));
    
    cpu.set_flag(Flag::C, false);
    EXPECT_FALSE(cpu.get_flag(Flag::C));
    
    cpu.set_flag(Flag::Z, true);
    EXPECT_TRUE(cpu.get_flag(Flag::Z));
    
    cpu.set_flag(Flag::N, true);
    EXPECT_TRUE(cpu.get_flag(Flag::N));
}

TEST_F(CPUTest, AllFlags) {
    cpu.set_status(0xFF);
    
    EXPECT_TRUE(cpu.get_flag(Flag::C));
    EXPECT_TRUE(cpu.get_flag(Flag::Z));
    EXPECT_TRUE(cpu.get_flag(Flag::I));
    EXPECT_TRUE(cpu.get_flag(Flag::D));
    EXPECT_TRUE(cpu.get_flag(Flag::B));
    EXPECT_TRUE(cpu.get_flag(Flag::U));
    EXPECT_TRUE(cpu.get_flag(Flag::V));
    EXPECT_TRUE(cpu.get_flag(Flag::N));
    
    cpu.set_status(0x00);
    
    EXPECT_FALSE(cpu.get_flag(Flag::C));
    EXPECT_FALSE(cpu.get_flag(Flag::Z));
    EXPECT_FALSE(cpu.get_flag(Flag::I));
    EXPECT_FALSE(cpu.get_flag(Flag::D));
    EXPECT_FALSE(cpu.get_flag(Flag::B));
    EXPECT_FALSE(cpu.get_flag(Flag::U));
    EXPECT_FALSE(cpu.get_flag(Flag::V));
    EXPECT_FALSE(cpu.get_flag(Flag::N));
}

// =============================================================================
// Load/Store Instructions
// =============================================================================

TEST_F(CPUTest, LDA_Immediate) {
    // LDA #$42
    load_program({0xA9, 0x42});
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x42);
    EXPECT_FALSE(cpu.get_flag(Flag::Z));
    EXPECT_FALSE(cpu.get_flag(Flag::N));
}

TEST_F(CPUTest, LDA_Zero) {
    // LDA #$00
    load_program({0xA9, 0x00});
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x00);
    EXPECT_TRUE(cpu.get_flag(Flag::Z));
    EXPECT_FALSE(cpu.get_flag(Flag::N));
}

TEST_F(CPUTest, LDA_Negative) {
    // LDA #$80
    load_program({0xA9, 0x80});
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x80);
    EXPECT_FALSE(cpu.get_flag(Flag::Z));
    EXPECT_TRUE(cpu.get_flag(Flag::N));
}

TEST_F(CPUTest, LDA_ZeroPage) {
    // LDA $10
    memory.write(0x0010, 0x55);
    load_program({0xA5, 0x10});
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x55);
}

TEST_F(CPUTest, LDX_Immediate) {
    // LDX #$42
    load_program({0xA2, 0x42});
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_x(), 0x42);
}

TEST_F(CPUTest, LDY_Immediate) {
    // LDY #$42
    load_program({0xA0, 0x42});
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_y(), 0x42);
}

TEST_F(CPUTest, STA_ZeroPage) {
    // STA $10
    cpu.set_a(0xAB);
    load_program({0x85, 0x10});
    
    cpu.step();
    
    EXPECT_EQ(memory.read(0x0010), 0xAB);
}

TEST_F(CPUTest, STX_ZeroPage) {
    // STX $10
    cpu.set_x(0xCD);
    load_program({0x86, 0x10});
    
    cpu.step();
    
    EXPECT_EQ(memory.read(0x0010), 0xCD);
}

TEST_F(CPUTest, STY_ZeroPage) {
    // STY $10
    cpu.set_y(0xEF);
    load_program({0x84, 0x10});
    
    cpu.step();
    
    EXPECT_EQ(memory.read(0x0010), 0xEF);
}

// =============================================================================
// Transfer Instructions
// =============================================================================

TEST_F(CPUTest, TAX) {
    cpu.set_a(0x42);
    load_program({0xAA});  // TAX
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_x(), 0x42);
}

TEST_F(CPUTest, TAY) {
    cpu.set_a(0x42);
    load_program({0xA8});  // TAY
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_y(), 0x42);
}

TEST_F(CPUTest, TXA) {
    cpu.set_x(0x42);
    load_program({0x8A});  // TXA
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x42);
}

TEST_F(CPUTest, TYA) {
    cpu.set_y(0x42);
    load_program({0x98});  // TYA
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x42);
}

TEST_F(CPUTest, TSX) {
    cpu.set_sp(0x80);
    load_program({0xBA});  // TSX
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_x(), 0x80);
}

TEST_F(CPUTest, TXS) {
    cpu.set_x(0x80);
    load_program({0x9A});  // TXS
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_sp(), 0x80);
}

// =============================================================================
// Arithmetic Instructions
// =============================================================================

TEST_F(CPUTest, ADC_NoCarry) {
    cpu.set_a(0x10);
    cpu.set_flag(Flag::C, false);
    load_program({0x69, 0x20});  // ADC #$20
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x30);
    EXPECT_FALSE(cpu.get_flag(Flag::C));
    EXPECT_FALSE(cpu.get_flag(Flag::V));
}

TEST_F(CPUTest, ADC_WithCarryIn) {
    cpu.set_a(0x10);
    cpu.set_flag(Flag::C, true);
    load_program({0x69, 0x20});  // ADC #$20
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x31);
}

TEST_F(CPUTest, ADC_CarryOut) {
    cpu.set_a(0xFF);
    cpu.set_flag(Flag::C, false);
    load_program({0x69, 0x01});  // ADC #$01
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x00);
    EXPECT_TRUE(cpu.get_flag(Flag::C));
    EXPECT_TRUE(cpu.get_flag(Flag::Z));
}

TEST_F(CPUTest, ADC_Overflow) {
    cpu.set_a(0x7F);  // +127
    cpu.set_flag(Flag::C, false);
    load_program({0x69, 0x01});  // ADC #$01
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x80);  // -128 (overflow!)
    EXPECT_TRUE(cpu.get_flag(Flag::V));
    EXPECT_TRUE(cpu.get_flag(Flag::N));
}

TEST_F(CPUTest, SBC_Simple) {
    cpu.set_a(0x50);
    cpu.set_flag(Flag::C, true);  // No borrow
    load_program({0xE9, 0x20});  // SBC #$20
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x30);
    EXPECT_TRUE(cpu.get_flag(Flag::C));  // No borrow
}

TEST_F(CPUTest, SBC_WithBorrow) {
    cpu.set_a(0x50);
    cpu.set_flag(Flag::C, false);  // Borrow
    load_program({0xE9, 0x20});  // SBC #$20
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x2F);
}

TEST_F(CPUTest, INX) {
    cpu.set_x(0x41);
    load_program({0xE8});  // INX
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_x(), 0x42);
}

TEST_F(CPUTest, INY) {
    cpu.set_y(0x41);
    load_program({0xC8});  // INY
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_y(), 0x42);
}

TEST_F(CPUTest, DEX) {
    cpu.set_x(0x42);
    load_program({0xCA});  // DEX
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_x(), 0x41);
}

TEST_F(CPUTest, DEY) {
    cpu.set_y(0x42);
    load_program({0x88});  // DEY
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_y(), 0x41);
}

TEST_F(CPUTest, INC_ZeroPage) {
    memory.write(0x0010, 0x41);
    load_program({0xE6, 0x10});  // INC $10
    
    cpu.step();
    
    EXPECT_EQ(memory.read(0x0010), 0x42);
}

TEST_F(CPUTest, DEC_ZeroPage) {
    memory.write(0x0010, 0x42);
    load_program({0xC6, 0x10});  // DEC $10
    
    cpu.step();
    
    EXPECT_EQ(memory.read(0x0010), 0x41);
}

// =============================================================================
// Logical Instructions
// =============================================================================

TEST_F(CPUTest, AND_Immediate) {
    cpu.set_a(0xFF);
    load_program({0x29, 0x0F});  // AND #$0F
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x0F);
}

TEST_F(CPUTest, ORA_Immediate) {
    cpu.set_a(0xF0);
    load_program({0x09, 0x0F});  // ORA #$0F
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0xFF);
}

TEST_F(CPUTest, EOR_Immediate) {
    cpu.set_a(0xFF);
    load_program({0x49, 0x0F});  // EOR #$0F
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0xF0);
}

// =============================================================================
// Shift/Rotate Instructions
// =============================================================================

TEST_F(CPUTest, ASL_Accumulator) {
    cpu.set_a(0x40);
    load_program({0x0A});  // ASL A
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x80);
    EXPECT_FALSE(cpu.get_flag(Flag::C));
}

TEST_F(CPUTest, ASL_CarryOut) {
    cpu.set_a(0x80);
    load_program({0x0A});  // ASL A
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x00);
    EXPECT_TRUE(cpu.get_flag(Flag::C));
}

TEST_F(CPUTest, LSR_Accumulator) {
    cpu.set_a(0x02);
    load_program({0x4A});  // LSR A
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x01);
    EXPECT_FALSE(cpu.get_flag(Flag::C));
}

TEST_F(CPUTest, LSR_CarryOut) {
    cpu.set_a(0x01);
    load_program({0x4A});  // LSR A
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x00);
    EXPECT_TRUE(cpu.get_flag(Flag::C));
}

TEST_F(CPUTest, ROL_Accumulator) {
    cpu.set_a(0x80);
    cpu.set_flag(Flag::C, false);
    load_program({0x2A});  // ROL A
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x00);
    EXPECT_TRUE(cpu.get_flag(Flag::C));
}

TEST_F(CPUTest, ROL_WithCarry) {
    cpu.set_a(0x00);
    cpu.set_flag(Flag::C, true);
    load_program({0x2A});  // ROL A
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x01);
    EXPECT_FALSE(cpu.get_flag(Flag::C));
}

TEST_F(CPUTest, ROR_Accumulator) {
    cpu.set_a(0x01);
    cpu.set_flag(Flag::C, false);
    load_program({0x6A});  // ROR A
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x00);
    EXPECT_TRUE(cpu.get_flag(Flag::C));
}

TEST_F(CPUTest, ROR_WithCarry) {
    cpu.set_a(0x00);
    cpu.set_flag(Flag::C, true);
    load_program({0x6A});  // ROR A
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x80);
    EXPECT_FALSE(cpu.get_flag(Flag::C));
}

// =============================================================================
// Compare Instructions
// =============================================================================

TEST_F(CPUTest, CMP_Equal) {
    cpu.set_a(0x42);
    load_program({0xC9, 0x42});  // CMP #$42
    
    cpu.step();
    
    EXPECT_TRUE(cpu.get_flag(Flag::Z));
    EXPECT_TRUE(cpu.get_flag(Flag::C));
    EXPECT_FALSE(cpu.get_flag(Flag::N));
}

TEST_F(CPUTest, CMP_Greater) {
    cpu.set_a(0x50);
    load_program({0xC9, 0x42});  // CMP #$42
    
    cpu.step();
    
    EXPECT_FALSE(cpu.get_flag(Flag::Z));
    EXPECT_TRUE(cpu.get_flag(Flag::C));
}

TEST_F(CPUTest, CMP_Less) {
    cpu.set_a(0x30);
    load_program({0xC9, 0x42});  // CMP #$42
    
    cpu.step();
    
    EXPECT_FALSE(cpu.get_flag(Flag::Z));
    EXPECT_FALSE(cpu.get_flag(Flag::C));
}

TEST_F(CPUTest, CPX_Equal) {
    cpu.set_x(0x42);
    load_program({0xE0, 0x42});  // CPX #$42
    
    cpu.step();
    
    EXPECT_TRUE(cpu.get_flag(Flag::Z));
    EXPECT_TRUE(cpu.get_flag(Flag::C));
}

TEST_F(CPUTest, CPY_Equal) {
    cpu.set_y(0x42);
    load_program({0xC0, 0x42});  // CPY #$42
    
    cpu.step();
    
    EXPECT_TRUE(cpu.get_flag(Flag::Z));
    EXPECT_TRUE(cpu.get_flag(Flag::C));
}

// =============================================================================
// Branch Instructions
// =============================================================================

TEST_F(CPUTest, BEQ_Taken) {
    cpu.set_flag(Flag::Z, true);
    load_program({0xF0, 0x05});  // BEQ +5
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_pc(), 0x0207);  // 0x0202 + 5
}

TEST_F(CPUTest, BEQ_NotTaken) {
    cpu.set_flag(Flag::Z, false);
    load_program({0xF0, 0x05});  // BEQ +5
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_pc(), 0x0202);  // Just after instruction
}

TEST_F(CPUTest, BNE_Taken) {
    cpu.set_flag(Flag::Z, false);
    load_program({0xD0, 0x05});  // BNE +5
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_pc(), 0x0207);
}

TEST_F(CPUTest, BCC_Taken) {
    cpu.set_flag(Flag::C, false);
    load_program({0x90, 0x05});  // BCC +5
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_pc(), 0x0207);
}

TEST_F(CPUTest, BCS_Taken) {
    cpu.set_flag(Flag::C, true);
    load_program({0xB0, 0x05});  // BCS +5
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_pc(), 0x0207);
}

TEST_F(CPUTest, BMI_Taken) {
    cpu.set_flag(Flag::N, true);
    load_program({0x30, 0x05});  // BMI +5
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_pc(), 0x0207);
}

TEST_F(CPUTest, BPL_Taken) {
    cpu.set_flag(Flag::N, false);
    load_program({0x10, 0x05});  // BPL +5
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_pc(), 0x0207);
}

TEST_F(CPUTest, BranchBackward) {
    cpu.set_flag(Flag::Z, true);
    load_program({0xF0, 0xFE});  // BEQ -2 (infinite loop)
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_pc(), 0x0200);  // Back to start
}

// =============================================================================
// Jump/Call Instructions
// =============================================================================

TEST_F(CPUTest, JMP_Absolute) {
    load_program({0x4C, 0x00, 0x03});  // JMP $0300
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_pc(), 0x0300);
}

TEST_F(CPUTest, JSR_RTS) {
    // JSR $0210
    load_program({0x20, 0x10, 0x02});
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_pc(), 0x0210);
    
    // Put RTS at $0210
    memory.write(0x0210, 0x60);  // RTS
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_pc(), 0x0203);  // Return to after JSR
}

// =============================================================================
// Stack Instructions
// =============================================================================

TEST_F(CPUTest, PHA_PLA) {
    cpu.set_a(0x42);
    load_program({0x48, 0xA9, 0x00, 0x68});  // PHA, LDA #$00, PLA
    
    cpu.step();  // PHA
    EXPECT_EQ(memory.read(0x01FD), 0x42);
    
    cpu.step();  // LDA #$00
    EXPECT_EQ(cpu.get_a(), 0x00);
    
    cpu.step();  // PLA
    EXPECT_EQ(cpu.get_a(), 0x42);
}

TEST_F(CPUTest, PHP_PLP) {
    cpu.set_status(0xFF);
    load_program({0x08, 0xA9, 0x00, 0x28});  // PHP, LDA #$00, PLP
    
    std::uint8_t original_status = cpu.get_status();
    
    cpu.step();  // PHP
    
    cpu.set_status(0x00);
    
    cpu.step();  // LDA #$00 (changes Z flag)
    cpu.step();  // PLP
    
    // B and U flags have special behavior
    EXPECT_EQ(cpu.get_status() & 0xCF, original_status & 0xCF);
}

// =============================================================================
// Flag Instructions
// =============================================================================

TEST_F(CPUTest, CLC) {
    cpu.set_flag(Flag::C, true);
    load_program({0x18});  // CLC
    
    cpu.step();
    
    EXPECT_FALSE(cpu.get_flag(Flag::C));
}

TEST_F(CPUTest, SEC) {
    cpu.set_flag(Flag::C, false);
    load_program({0x38});  // SEC
    
    cpu.step();
    
    EXPECT_TRUE(cpu.get_flag(Flag::C));
}

TEST_F(CPUTest, CLI) {
    cpu.set_flag(Flag::I, true);
    load_program({0x58});  // CLI
    
    cpu.step();
    
    EXPECT_FALSE(cpu.get_flag(Flag::I));
}

TEST_F(CPUTest, SEI) {
    cpu.set_flag(Flag::I, false);
    load_program({0x78});  // SEI
    
    cpu.step();
    
    EXPECT_TRUE(cpu.get_flag(Flag::I));
}

TEST_F(CPUTest, CLV) {
    cpu.set_flag(Flag::V, true);
    load_program({0xB8});  // CLV
    
    cpu.step();
    
    EXPECT_FALSE(cpu.get_flag(Flag::V));
}

TEST_F(CPUTest, CLD) {
    cpu.set_flag(Flag::D, true);
    load_program({0xD8});  // CLD
    
    cpu.step();
    
    EXPECT_FALSE(cpu.get_flag(Flag::D));
}

TEST_F(CPUTest, SED) {
    cpu.set_flag(Flag::D, false);
    load_program({0xF8});  // SED
    
    cpu.step();
    
    EXPECT_TRUE(cpu.get_flag(Flag::D));
}

// =============================================================================
// Misc Instructions
// =============================================================================

TEST_F(CPUTest, NOP) {
    std::uint16_t pc_before = cpu.get_pc();
    load_program({0xEA});  // NOP
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_pc(), pc_before + 1);
}

TEST_F(CPUTest, BIT_ZeroPage) {
    memory.write(0x0010, 0xC0);  // Bits 7 and 6 set
    cpu.set_a(0x00);
    load_program({0x24, 0x10});  // BIT $10
    
    cpu.step();
    
    EXPECT_TRUE(cpu.get_flag(Flag::Z));   // A & M == 0
    EXPECT_TRUE(cpu.get_flag(Flag::N));   // Bit 7 of M
    EXPECT_TRUE(cpu.get_flag(Flag::V));   // Bit 6 of M
}

// =============================================================================
// Addressing Mode Tests
// =============================================================================

TEST_F(CPUTest, ZeroPageX) {
    cpu.set_x(0x05);
    memory.write(0x0015, 0x42);
    load_program({0xB5, 0x10});  // LDA $10,X
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x42);
}

TEST_F(CPUTest, ZeroPageX_Wrap) {
    cpu.set_x(0x10);
    memory.write(0x0005, 0x42);  // $F5 + $10 = $105 -> wraps to $05
    load_program({0xB5, 0xF5});  // LDA $F5,X
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x42);
}

TEST_F(CPUTest, AbsoluteX) {
    cpu.set_x(0x05);
    memory.write(0x0305, 0x42);
    load_program({0xBD, 0x00, 0x03});  // LDA $0300,X
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x42);
}

TEST_F(CPUTest, AbsoluteY) {
    cpu.set_y(0x05);
    memory.write(0x0305, 0x42);
    load_program({0xB9, 0x00, 0x03});  // LDA $0300,Y
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x42);
}

TEST_F(CPUTest, IndirectX) {
    cpu.set_x(0x04);
    memory.write(0x0024, 0x00);  // Low byte of address
    memory.write(0x0025, 0x03);  // High byte -> $0300
    memory.write(0x0300, 0x42);
    load_program({0xA1, 0x20});  // LDA ($20,X)
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x42);
}

TEST_F(CPUTest, IndirectY) {
    cpu.set_y(0x10);
    memory.write(0x0020, 0x00);  // Low byte
    memory.write(0x0021, 0x03);  // High byte -> $0300
    memory.write(0x0310, 0x42);  // $0300 + Y
    load_program({0xB1, 0x20});  // LDA ($20),Y
    
    cpu.step();
    
    EXPECT_EQ(cpu.get_a(), 0x42);
}

// =============================================================================
// Cycle Counting Tests
// =============================================================================

TEST_F(CPUTest, LDA_Immediate_Cycles) {
    load_program({0xA9, 0x42});  // LDA #$42
    
    int cycles = cpu.step();
    
    EXPECT_EQ(cycles, 2);
}

TEST_F(CPUTest, LDA_ZeroPage_Cycles) {
    load_program({0xA5, 0x10});  // LDA $10
    
    int cycles = cpu.step();
    
    EXPECT_EQ(cycles, 3);
}

TEST_F(CPUTest, JMP_Cycles) {
    load_program({0x4C, 0x00, 0x03});  // JMP $0300
    
    int cycles = cpu.step();
    
    EXPECT_EQ(cycles, 3);
}

// =============================================================================
// Interrupt Tests
// =============================================================================

TEST_F(CPUTest, NMI) {
    // Set NMI vector
    memory.write(0x00FA, 0x00);  // Can't write to $FFFA, test with mock later
    memory.write(0x00FB, 0x80);
    
    cpu.set_pc(0x0200);
    std::uint8_t status_before = cpu.get_status();
    
    // For now just verify NMI doesn't crash
    cpu.nmi();
    
    // Stack should have been pushed
    EXPECT_NE(cpu.get_sp(), 0xFD);
}

TEST_F(CPUTest, IRQ_WhenEnabled) {
    cpu.set_flag(Flag::I, false);  // Enable interrupts
    cpu.set_pc(0x0200);
    
    cpu.irq();
    
    // IRQ should have been processed
    EXPECT_NE(cpu.get_sp(), 0xFD);
    EXPECT_TRUE(cpu.get_flag(Flag::I));  // I flag set after IRQ
}

TEST_F(CPUTest, IRQ_WhenDisabled) {
    cpu.set_flag(Flag::I, true);  // Disable interrupts
    cpu.set_pc(0x0200);
    std::uint8_t sp_before = cpu.get_sp();
    
    cpu.irq();
    
    // IRQ should have been ignored
    EXPECT_EQ(cpu.get_sp(), sp_before);
    EXPECT_EQ(cpu.get_pc(), 0x0200);
}

// =============================================================================
// Integration Tests
// =============================================================================

TEST_F(CPUTest, SimpleLoop) {
    // LDX #$05
    // DEX
    // BNE -2
    load_program({0xA2, 0x05, 0xCA, 0xD0, 0xFD});
    
    cpu.step();  // LDX #$05
    EXPECT_EQ(cpu.get_x(), 0x05);
    
    for (int i = 4; i >= 0; i--) {
        cpu.step();  // DEX
        EXPECT_EQ(cpu.get_x(), i);
        
        cpu.step();  // BNE
        if (i > 0) {
            EXPECT_EQ(cpu.get_pc(), 0x0202);  // Loop back
        }
    }
    
    EXPECT_EQ(cpu.get_x(), 0x00);
}

TEST_F(CPUTest, MemoryCopy) {
    // Copy 3 bytes from $10 to $20
    // $0200: LDX #$00
    // $0202: LDA $10,X
    // $0204: STA $20,X
    // $0206: INX
    // $0207: CPX #$03
    // $0209: BNE $0202 (-9 bytes from $020B = $0202)
    
    memory.write(0x0010, 0xAA);
    memory.write(0x0011, 0xBB);
    memory.write(0x0012, 0xCC);
    
    // LDX #$00, LDA $10,X, STA $20,X, INX, CPX #$03, BNE -9
    load_program({0xA2, 0x00, 0xB5, 0x10, 0x95, 0x20, 0xE8, 0xE0, 0x03, 0xD0, 0xF7});
    
    // Execute until done (max 50 iterations for safety)
    for (int i = 0; i < 50; i++) {
        cpu.step();
        if (cpu.get_x() == 3 && !cpu.get_flag(Flag::Z)) break;
        if (cpu.get_pc() > 0x020B) break;
    }
    
    EXPECT_EQ(memory.read(0x0020), 0xAA);
    EXPECT_EQ(memory.read(0x0021), 0xBB);
    EXPECT_EQ(memory.read(0x0022), 0xCC);
}
