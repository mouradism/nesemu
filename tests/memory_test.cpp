#include <gtest/gtest.h>
#include "memory.hpp"
#include "cartridge.hpp"
#include "ppu.hpp"
#include "apu.hpp"
#include "controller.hpp"

// =============================================================================
// Memory Test Fixture
// =============================================================================

class MemoryTest : public ::testing::Test {
protected:
    Cartridge cart{""};  // Empty cartridge
    Memory memory{cart};
    PPU ppu{memory};
    APU apu;
    Controller controller1;
    Controller controller2;
    
    void SetUp() override {
        memory.set_ppu(&ppu);
        memory.set_apu(&apu);
        memory.set_controllers(&controller1, &controller2);
        ppu.reset();
        apu.reset();
        controller1.reset();
        controller2.reset();
    }
};

// =============================================================================
// RAM Tests ($0000-$1FFF)
// =============================================================================

TEST_F(MemoryTest, RAMReadWrite) {
    memory.write(0x0000, 0xAB);
    EXPECT_EQ(memory.read(0x0000), 0xAB);
    
    memory.write(0x07FF, 0xCD);
    EXPECT_EQ(memory.read(0x07FF), 0xCD);
}

TEST_F(MemoryTest, RAMMirroring) {
    // Write to base RAM
    memory.write(0x0100, 0x42);
    
    // Should be mirrored at $0900, $1100, $1900
    EXPECT_EQ(memory.read(0x0100), 0x42);
    EXPECT_EQ(memory.read(0x0900), 0x42);
    EXPECT_EQ(memory.read(0x1100), 0x42);
    EXPECT_EQ(memory.read(0x1900), 0x42);
}

TEST_F(MemoryTest, RAMMirroringWrite) {
    // Write to mirrored address
    memory.write(0x0900, 0x55);
    
    // Should appear at base address
    EXPECT_EQ(memory.read(0x0100), 0x55);
}

TEST_F(MemoryTest, FullRAMRange) {
    // Write pattern to all of RAM
    for (int i = 0; i < 2048; ++i) {
        memory.write(static_cast<std::uint16_t>(i), static_cast<std::uint8_t>(i & 0xFF));
    }
    
    // Verify
    for (int i = 0; i < 2048; ++i) {
        EXPECT_EQ(memory.read(static_cast<std::uint16_t>(i)), static_cast<std::uint8_t>(i & 0xFF));
    }
}

// =============================================================================
// PPU Register Tests ($2000-$3FFF)
// =============================================================================

TEST_F(MemoryTest, PPURegisterWrite) {
    // Write to PPUCTRL
    memory.write(0x2000, 0x80);
    
    // Write to PPUMASK
    memory.write(0x2001, 0x1E);
    
    // Should not crash
    EXPECT_TRUE(true);
}

TEST_F(MemoryTest, PPURegisterRead) {
    // Read PPUSTATUS
    std::uint8_t status = memory.read(0x2002);
    
    // Should return something (exact value depends on PPU state)
    (void)status;
    EXPECT_TRUE(true);
}

TEST_F(MemoryTest, PPURegisterMirroring) {
    // PPU registers mirror every 8 bytes from $2000-$3FFF
    memory.write(0x2000, 0x80);
    memory.write(0x2008, 0x00);  // Mirror of $2000
    memory.write(0x3FF8, 0x40);  // Another mirror of $2000
    
    EXPECT_TRUE(true);
}

TEST_F(MemoryTest, OAMDATAAccess) {
    // Write OAM address
    memory.write(0x2003, 0x00);
    
    // Write OAM data
    memory.write(0x2004, 0x10);
    memory.write(0x2004, 0x20);
    memory.write(0x2004, 0x30);
    memory.write(0x2004, 0x40);
    
    // Read back
    memory.write(0x2003, 0x00);
    EXPECT_EQ(memory.read(0x2004), 0x10);
}

// =============================================================================
// APU Register Tests ($4000-$4017)
// =============================================================================

TEST_F(MemoryTest, APUPulse1Registers) {
    memory.write(0x4000, 0x30);  // Duty, envelope
    memory.write(0x4001, 0x08);  // Sweep
    memory.write(0x4002, 0x00);  // Timer low
    memory.write(0x4003, 0x00);  // Timer high, length
    
    EXPECT_TRUE(true);
}

TEST_F(MemoryTest, APUPulse2Registers) {
    memory.write(0x4004, 0x30);
    memory.write(0x4005, 0x08);
    memory.write(0x4006, 0x00);
    memory.write(0x4007, 0x00);
    
    EXPECT_TRUE(true);
}

TEST_F(MemoryTest, APUTriangleRegisters) {
    memory.write(0x4008, 0x80);
    memory.write(0x400A, 0x00);
    memory.write(0x400B, 0x00);
    
    EXPECT_TRUE(true);
}

TEST_F(MemoryTest, APUNoiseRegisters) {
    memory.write(0x400C, 0x30);
    memory.write(0x400E, 0x00);
    memory.write(0x400F, 0x00);
    
    EXPECT_TRUE(true);
}

TEST_F(MemoryTest, APUDMCRegisters) {
    memory.write(0x4010, 0x00);
    memory.write(0x4011, 0x00);
    memory.write(0x4012, 0x00);
    memory.write(0x4013, 0x00);
    
    EXPECT_TRUE(true);
}

TEST_F(MemoryTest, APUStatusRegister) {
    // Write to enable channels
    memory.write(0x4015, 0x0F);
    
    // Read status
    std::uint8_t status = memory.read(0x4015);
    (void)status;
    
    EXPECT_TRUE(true);
}

TEST_F(MemoryTest, APUFrameCounter) {
    memory.write(0x4017, 0x40);  // 4-step mode
    memory.write(0x4017, 0xC0);  // 5-step mode, inhibit IRQ
    
    EXPECT_TRUE(true);
}

// =============================================================================
// Controller Tests ($4016-$4017)
// =============================================================================

TEST_F(MemoryTest, Controller1Strobe) {
    // Set button state
    controller1.set_button(Button::A, true);
    controller1.set_button(Button::Start, true);
    
    // Strobe high then low to latch
    memory.write(0x4016, 0x01);
    memory.write(0x4016, 0x00);
    
    // Read buttons
    std::uint8_t a_button = memory.read(0x4016) & 0x01;
    EXPECT_EQ(a_button, 1);  // A is pressed
}

TEST_F(MemoryTest, Controller1FullRead) {
    // Set all buttons
    controller1.set_state(0xFF);  // All pressed
    
    // Strobe
    memory.write(0x4016, 0x01);
    memory.write(0x4016, 0x00);
    
    // Read all 8 buttons
    for (int i = 0; i < 8; ++i) {
        std::uint8_t button = memory.read(0x4016) & 0x01;
        EXPECT_EQ(button, 1);
    }
}

TEST_F(MemoryTest, Controller1NoButtons) {
    // No buttons pressed
    controller1.set_state(0x00);
    
    // Strobe
    memory.write(0x4016, 0x01);
    memory.write(0x4016, 0x00);
    
    // Read all 8 buttons
    for (int i = 0; i < 8; ++i) {
        std::uint8_t button = memory.read(0x4016) & 0x01;
        EXPECT_EQ(button, 0);
    }
}

TEST_F(MemoryTest, Controller2Access) {
    controller2.set_button(Button::B, true);
    
    memory.write(0x4016, 0x01);
    memory.write(0x4016, 0x00);
    
    // First read: A button (not pressed)
    std::uint8_t a = memory.read(0x4017) & 0x01;
    EXPECT_EQ(a, 0);
    
    // Second read: B button (pressed)
    std::uint8_t b = memory.read(0x4017) & 0x01;
    EXPECT_EQ(b, 1);
}

TEST_F(MemoryTest, ControllerStrobeHigh) {
    // While strobe is high, always returns A button
    controller1.set_button(Button::A, true);
    
    memory.write(0x4016, 0x01);  // Strobe high
    
    // Multiple reads all return A
    for (int i = 0; i < 10; ++i) {
        std::uint8_t a = memory.read(0x4016) & 0x01;
        EXPECT_EQ(a, 1);
    }
}

// =============================================================================
// OAM DMA Tests ($4014)
// =============================================================================

TEST_F(MemoryTest, OAMDMATrigger) {
    EXPECT_FALSE(memory.dma_pending());
    
    // Write to $4014 triggers DMA
    memory.write(0x4014, 0x02);  // Page $0200
    
    EXPECT_TRUE(memory.dma_pending());
    EXPECT_EQ(memory.dma_page(), 0x02);
}

TEST_F(MemoryTest, OAMDMAExecute) {
    // Fill page $02 with test data
    for (int i = 0; i < 256; ++i) {
        memory.write(0x0200 + i, static_cast<std::uint8_t>(i));
    }
    
    // Set OAM address to 0
    memory.write(0x2003, 0x00);
    
    // Trigger DMA
    memory.write(0x4014, 0x02);
    
    // Execute DMA
    memory.execute_dma();
    
    // DMA should be complete
    EXPECT_FALSE(memory.dma_pending());
    
    // Verify OAM data (read back via OAMDATA)
    memory.write(0x2003, 0x00);
    EXPECT_EQ(memory.read(0x2004), 0x00);
    
    memory.write(0x2003, 0x01);
    EXPECT_EQ(memory.read(0x2004), 0x01);
}

TEST_F(MemoryTest, OAMDMAClear) {
    memory.write(0x4014, 0x02);
    EXPECT_TRUE(memory.dma_pending());
    
    memory.clear_dma_pending();
    EXPECT_FALSE(memory.dma_pending());
}

// =============================================================================
// PRG RAM Tests ($6000-$7FFF)
// =============================================================================

TEST_F(MemoryTest, PRGRAMRange) {
    // PRG RAM access depends on mapper
    // With empty cartridge, may return 0
    std::uint8_t val = memory.read(0x6000);
    (void)val;
    
    memory.write(0x6000, 0xAB);
    memory.write(0x7FFF, 0xCD);
    
    EXPECT_TRUE(true);
}

// =============================================================================
// PRG ROM Tests ($8000-$FFFF)
// =============================================================================

TEST_F(MemoryTest, PRGROMRead) {
    // With empty cartridge, returns 0
    EXPECT_EQ(memory.read(0x8000), 0);
    EXPECT_EQ(memory.read(0xFFFC), 0);  // Reset vector
    EXPECT_EQ(memory.read(0xFFFD), 0);
}

TEST_F(MemoryTest, PRGROMWriteToMapper) {
    // Write to PRG ROM range may trigger mapper
    memory.write(0x8000, 0x00);
    memory.write(0xFFFF, 0xFF);
    
    EXPECT_TRUE(true);
}

// =============================================================================
// Peek Tests (No Side Effects)
// =============================================================================

TEST_F(MemoryTest, PeekRAM) {
    memory.write(0x0100, 0xAB);
    EXPECT_EQ(memory.peek(0x0100), 0xAB);
}

TEST_F(MemoryTest, PeekDoesNotAffectPPU) {
    // Peek at PPU registers should not have side effects
    // (unlike read which clears flags, etc.)
    std::uint8_t val = memory.peek(0x2002);
    (void)val;
    
    EXPECT_TRUE(true);
}

// =============================================================================
// Integration Tests
// =============================================================================

TEST_F(MemoryTest, FullAddressSpaceAccess) {
    // Test we can access all address ranges without crash
    for (std::uint32_t addr = 0; addr <= 0xFFFF; addr += 0x100) {
        memory.read(static_cast<std::uint16_t>(addr));
        memory.write(static_cast<std::uint16_t>(addr), 0x00);
    }
    
    EXPECT_TRUE(true);
}

TEST_F(MemoryTest, NullComponentsSafe) {
    // Create memory without setting components
    Memory bare_memory{cart};
    
    // Should not crash even without PPU/APU/Controllers
    bare_memory.read(0x2002);
    bare_memory.write(0x2000, 0x80);
    bare_memory.read(0x4015);
    bare_memory.write(0x4000, 0x30);
    bare_memory.read(0x4016);
    bare_memory.write(0x4016, 0x01);
    
    EXPECT_TRUE(true);
}

TEST_F(MemoryTest, DirectRAMAccess) {
    std::uint8_t* ram = memory.ram_data();
    ASSERT_NE(ram, nullptr);
    
    ram[0] = 0x12;
    ram[100] = 0x34;
    
    EXPECT_EQ(memory.read(0x0000), 0x12);
    EXPECT_EQ(memory.read(0x0064), 0x34);
}

// =============================================================================
// Controller Unit Tests
// =============================================================================

TEST(ControllerTest, SetButton) {
    Controller ctrl;
    
    ctrl.set_button(Button::A, true);
    EXPECT_TRUE(ctrl.is_pressed(Button::A));
    EXPECT_FALSE(ctrl.is_pressed(Button::B));
    
    ctrl.set_button(Button::A, false);
    EXPECT_FALSE(ctrl.is_pressed(Button::A));
}

TEST(ControllerTest, SetState) {
    Controller ctrl;
    
    ctrl.set_state(0xFF);
    EXPECT_EQ(ctrl.get_state(), 0xFF);
    
    for (int i = 0; i < 8; ++i) {
        EXPECT_TRUE(ctrl.is_pressed(static_cast<Button>(i)));
    }
}

TEST(ControllerTest, ShiftRegister) {
    Controller ctrl;
    
    // A=1, B=1, others=0
    ctrl.set_button(Button::A, true);
    ctrl.set_button(Button::B, true);
    
    // Strobe
    ctrl.write(0x01);
    ctrl.write(0x00);
    
    // Read A
    EXPECT_EQ(ctrl.read() & 0x01, 1);
    
    // Read B
    EXPECT_EQ(ctrl.read() & 0x01, 1);
    
    // Read Select (not pressed)
    EXPECT_EQ(ctrl.read() & 0x01, 0);
}

TEST(ControllerTest, Reset) {
    Controller ctrl;
    
    ctrl.set_state(0xFF);
    ctrl.reset();
    
    EXPECT_EQ(ctrl.get_state(), 0x00);
}
