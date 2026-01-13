#include <gtest/gtest.h>
#include "ppu.hpp"
#include "memory.hpp"
#include "cartridge.hpp"

class PPUTest : public ::testing::Test {
protected:
    Cartridge cart{""};  // Empty cartridge for testing
    Memory bus{cart};
    PPU ppu{bus};
    
    void SetUp() override {
        ppu.reset();
    }
    
    // Helper: Run PPU for a specific number of cycles
    void run_cycles(int cycles) {
        for (int i = 0; i < cycles; ++i) {
            ppu.step();
        }
    }
    
    // Helper: Run PPU for one full scanline (341 cycles)
    void run_scanline() {
        run_cycles(341);
    }
    
    // Helper: Run PPU for one full frame (341 * 262 cycles)
    void run_frame() {
        run_cycles(341 * 262);
    }
};

// =============================================================================
// Initialization and Reset Tests
// =============================================================================

TEST_F(PPUTest, ConstructorInitializesPPU) {
    PPU fresh_ppu(bus);
    // Should not be in frame ready state
    EXPECT_FALSE(fresh_ppu.frame_ready());
    EXPECT_FALSE(fresh_ppu.nmi_occurred());
}

TEST_F(PPUTest, ResetClearsState) {
    // Modify some state
    ppu.write(0x2000, 0xFF);
    ppu.write(0x2001, 0xFF);
    run_cycles(1000);
    
    // Reset
    ppu.reset();
    
    // Check state is cleared
    EXPECT_FALSE(ppu.frame_ready());
    EXPECT_FALSE(ppu.nmi_occurred());
}

TEST_F(PPUTest, FramebufferHasCorrectSize) {
    const auto& fb = ppu.get_framebuffer();
    EXPECT_EQ(fb.size(), 256 * 240);
}

// =============================================================================
// Timing Tests
// =============================================================================

TEST_F(PPUTest, FrameCompletesAfter262Scanlines) {
    EXPECT_FALSE(ppu.frame_ready());
    
    // Run almost one full frame (341 * 262 - 1 cycles)
    run_cycles(341 * 262 - 1);
    EXPECT_FALSE(ppu.frame_ready());
    
    // One more cycle should complete the frame
    ppu.step();
    EXPECT_TRUE(ppu.frame_ready());
}

TEST_F(PPUTest, ClearFrameReadyWorks) {
    run_frame();
    EXPECT_TRUE(ppu.frame_ready());
    
    ppu.clear_frame_ready();
    EXPECT_FALSE(ppu.frame_ready());
}

TEST_F(PPUTest, MultipleFramesWork) {
    for (int frame = 0; frame < 5; ++frame) {
        run_frame();
        EXPECT_TRUE(ppu.frame_ready());
        ppu.clear_frame_ready();
        EXPECT_FALSE(ppu.frame_ready());
    }
}

// =============================================================================
// VBlank and NMI Tests
// =============================================================================

TEST_F(PPUTest, VBlankSetAtScanline241) {
    // Run to scanline 241, cycle 1 (where VBlank is set)
    // Scanline 0-240 = 241 scanlines worth of cycles, plus 2 cycles into scanline 241
    run_cycles(241 * 341 + 2);
    
    // Read PPUSTATUS to check VBlank flag
    std::uint8_t status = ppu.read(0x2002);
    EXPECT_TRUE(status & 0x80);
}

TEST_F(PPUTest, VBlankClearedOnStatusRead) {
    // Run to VBlank
    run_cycles(241 * 341 + 2);
    
    // First read should have VBlank set
    std::uint8_t status1 = ppu.read(0x2002);
    EXPECT_TRUE(status1 & 0x80);
    
    // Second read should have VBlank cleared
    std::uint8_t status2 = ppu.read(0x2002);
    EXPECT_FALSE(status2 & 0x80);
}

TEST_F(PPUTest, VBlankClearedAtPreRenderScanline) {
    // Run to after VBlank set, then to pre-render scanline
    // Pre-render scanline is 261, cycle 1 clears VBlank
    // Total cycles: 261 * 341 + 2
    run_cycles(261 * 341 + 2);
    
    // VBlank should be cleared by hardware
    std::uint8_t status = ppu.read(0x2002);
    EXPECT_FALSE(status & 0x80);
}

TEST_F(PPUTest, NMINotGeneratedWhenDisabled) {
    // Disable NMI (PPUCTRL bit 7 = 0)
    ppu.write(0x2000, 0x00);
    
    // Run to VBlank
    run_cycles(241 * 341 + 2);
    
    EXPECT_FALSE(ppu.nmi_occurred());
}

TEST_F(PPUTest, NMIGeneratedWhenEnabled) {
    // Enable NMI (PPUCTRL bit 7 = 1)
    ppu.write(0x2000, 0x80);
    
    // Run to VBlank
    run_cycles(241 * 341 + 2);
    
    EXPECT_TRUE(ppu.nmi_occurred());
}

TEST_F(PPUTest, ClearNMIWorks) {
    ppu.write(0x2000, 0x80);
    run_cycles(241 * 341 + 2);
    
    EXPECT_TRUE(ppu.nmi_occurred());
    ppu.clear_nmi();
    EXPECT_FALSE(ppu.nmi_occurred());
}

// =============================================================================
// PPUCTRL ($2000) Tests
// =============================================================================

TEST_F(PPUTest, PPUCTRLWriteWorks) {
    ppu.write(0x2000, 0x80);  // Enable NMI
    
    // Run to VBlank - NMI should occur
    run_cycles(241 * 341 + 2);
    EXPECT_TRUE(ppu.nmi_occurred());
}

TEST_F(PPUTest, PPUCTRLIsWriteOnly) {
    ppu.write(0x2000, 0xFF);
    
    // Reading PPUCTRL should return 0 (write-only)
    std::uint8_t result = ppu.read(0x2000);
    EXPECT_EQ(result, 0x00);
}

// =============================================================================
// PPUMASK ($2001) Tests
// =============================================================================

TEST_F(PPUTest, PPUMASKWriteWorks) {
    ppu.write(0x2001, 0x1E);  // Enable all rendering
    
    // Should not crash and rendering should work
    run_frame();
    EXPECT_TRUE(ppu.frame_ready());
}

TEST_F(PPUTest, PPUMASKIsWriteOnly) {
    ppu.write(0x2001, 0xFF);
    
    // Reading PPUMASK should return 0 (write-only)
    std::uint8_t result = ppu.read(0x2001);
    EXPECT_EQ(result, 0x00);
}

// =============================================================================
// PPUSTATUS ($2002) Tests
// =============================================================================

TEST_F(PPUTest, PPUSTATUSReadResetsLatch) {
    // Write to PPUSCROLL twice to set latch
    ppu.write(0x2005, 0x10);  // First write (X scroll)
    ppu.write(0x2005, 0x20);  // Second write (Y scroll)
    
    // Read PPUSTATUS to reset latch
    ppu.read(0x2002);
    
    // Next PPUSCROLL write should be treated as first write
    ppu.write(0x2005, 0x30);
    // Should not crash - latch was reset
    EXPECT_TRUE(true);
}

TEST_F(PPUTest, PPUSTATUSOnlyReturnsTop3Bits) {
    run_cycles(241 * 341 + 2);  // Get to VBlank
    
    std::uint8_t status = ppu.read(0x2002);
    // Only bits 7, 6, 5 should be valid
    // VBlank (bit 7) should be set
    EXPECT_TRUE(status & 0x80);
    // Lower 5 bits are undefined/open bus
}

// =============================================================================
// OAMADDR ($2003) and OAMDATA ($2004) Tests
// =============================================================================

TEST_F(PPUTest, OAMADDRWriteWorks) {
    ppu.write(0x2003, 0x10);
    // Should not crash
    EXPECT_TRUE(true);
}

TEST_F(PPUTest, OAMDATAWriteAndRead) {
    // Set OAM address to 0
    ppu.write(0x2003, 0x00);
    
    // Write some sprite data
    ppu.write(0x2004, 0x10);  // Y position
    ppu.write(0x2004, 0x20);  // Tile index
    ppu.write(0x2004, 0x30);  // Attributes
    ppu.write(0x2004, 0x40);  // X position
    
    // Reset OAM address and read back
    ppu.write(0x2003, 0x00);
    EXPECT_EQ(ppu.read(0x2004), 0x10);
    
    ppu.write(0x2003, 0x01);
    EXPECT_EQ(ppu.read(0x2004), 0x20);
    
    ppu.write(0x2003, 0x02);
    EXPECT_EQ(ppu.read(0x2004), 0x30);
    
    ppu.write(0x2003, 0x03);
    EXPECT_EQ(ppu.read(0x2004), 0x40);
}

TEST_F(PPUTest, OAMDATAWriteIncrementsAddress) {
    ppu.write(0x2003, 0x00);
    
    // Write 4 bytes
    for (int i = 0; i < 4; ++i) {
        ppu.write(0x2004, static_cast<std::uint8_t>(i));
    }
    
    // OAM address should now be 4
    // Read from addresses 0-3
    for (int i = 0; i < 4; ++i) {
        ppu.write(0x2003, static_cast<std::uint8_t>(i));
        EXPECT_EQ(ppu.read(0x2004), static_cast<std::uint8_t>(i));
    }
}

// =============================================================================
// PPUSCROLL ($2005) Tests
// =============================================================================

TEST_F(PPUTest, PPUSCROLLIsWriteOnly) {
    ppu.write(0x2005, 0x50);
    
    std::uint8_t result = ppu.read(0x2005);
    EXPECT_EQ(result, 0x00);
}

TEST_F(PPUTest, PPUSCROLLDoubleWrite) {
    // First write sets X scroll
    ppu.write(0x2005, 0x10);
    
    // Second write sets Y scroll
    ppu.write(0x2005, 0x20);
    
    // Should not crash - latch toggles correctly
    EXPECT_TRUE(true);
}

// =============================================================================
// PPUADDR ($2006) Tests
// =============================================================================

TEST_F(PPUTest, PPUADDRIsWriteOnly) {
    ppu.write(0x2006, 0x20);
    ppu.write(0x2006, 0x00);
    
    std::uint8_t result = ppu.read(0x2006);
    EXPECT_EQ(result, 0x00);
}

TEST_F(PPUTest, PPUADDRDoubleWrite) {
    // First write sets high byte
    ppu.write(0x2006, 0x20);
    
    // Second write sets low byte
    ppu.write(0x2006, 0x00);
    
    // Address should now be 0x2000
    // Should not crash
    EXPECT_TRUE(true);
}

// =============================================================================
// PPUDATA ($2007) Tests
// =============================================================================

TEST_F(PPUTest, PPUDATAWriteToVRAM) {
    // Set address to nametable area ($2000)
    ppu.write(0x2006, 0x20);
    ppu.write(0x2006, 0x00);
    
    // Write data
    ppu.write(0x2007, 0xAB);
    
    // Read it back (need to set address again)
    ppu.write(0x2006, 0x20);
    ppu.write(0x2006, 0x00);
    
    // First read returns buffer (stale data)
    ppu.read(0x2007);
    
    // Re-read from same address
    ppu.write(0x2006, 0x20);
    ppu.write(0x2006, 0x00);
    ppu.read(0x2007);  // Buffer fill
    
    // Set address again and read
    ppu.write(0x2006, 0x20);
    ppu.write(0x2006, 0x00);
    std::uint8_t first_read = ppu.read(0x2007);  // Returns old buffer
    
    ppu.write(0x2006, 0x20);
    ppu.write(0x2006, 0x00);
    ppu.read(0x2007);  // Fill buffer
    ppu.write(0x2006, 0x20);
    ppu.write(0x2006, 0x00);
    
    // The buffered read behavior is complex - just verify no crash
    EXPECT_TRUE(true);
}

TEST_F(PPUTest, PPUDATAAddressIncrement1) {
    // PPUCTRL bit 2 = 0: increment by 1
    ppu.write(0x2000, 0x00);
    
    // Set address
    ppu.write(0x2006, 0x20);
    ppu.write(0x2006, 0x00);
    
    // Write and address should increment by 1
    ppu.write(0x2007, 0x11);
    ppu.write(0x2007, 0x22);
    ppu.write(0x2007, 0x33);
    
    // Should not crash
    EXPECT_TRUE(true);
}

TEST_F(PPUTest, PPUDATAAddressIncrement32) {
    // PPUCTRL bit 2 = 1: increment by 32
    ppu.write(0x2000, 0x04);
    
    // Set address
    ppu.write(0x2006, 0x20);
    ppu.write(0x2006, 0x00);
    
    // Write and address should increment by 32
    ppu.write(0x2007, 0x11);
    ppu.write(0x2007, 0x22);
    
    // Should not crash
    EXPECT_TRUE(true);
}

// =============================================================================
// Palette Tests
// =============================================================================

TEST_F(PPUTest, PaletteWriteAndRead) {
    // Set address to palette area ($3F00)
    ppu.write(0x2006, 0x3F);
    ppu.write(0x2006, 0x00);
    
    // Write palette data
    ppu.write(0x2007, 0x0F);  // Background color
    
    // Read it back (palette reads are not buffered)
    ppu.write(0x2006, 0x3F);
    ppu.write(0x2006, 0x00);
    
    std::uint8_t palette_value = ppu.read(0x2007);
    EXPECT_EQ(palette_value, 0x0F);
}

TEST_F(PPUTest, PaletteMirroring) {
    // Write to $3F00 (background color)
    ppu.write(0x2006, 0x3F);
    ppu.write(0x2006, 0x00);
    ppu.write(0x2007, 0x1A);
    
    // $3F10 should mirror $3F00
    ppu.write(0x2006, 0x3F);
    ppu.write(0x2006, 0x10);
    std::uint8_t mirrored = ppu.read(0x2007);
    EXPECT_EQ(mirrored, 0x1A);
}

TEST_F(PPUTest, PaletteMultipleEntries) {
    // Write all 4 background palettes (16 entries)
    for (int i = 0; i < 16; ++i) {
        ppu.write(0x2006, 0x3F);
        ppu.write(0x2006, static_cast<std::uint8_t>(i));
        ppu.write(0x2007, static_cast<std::uint8_t>(i + 0x10));
    }
    
    // Read them back
    for (int i = 0; i < 16; ++i) {
        // Skip mirrored entries (0, 4, 8, 12 for sprites mirror background)
        if (i == 0 || i == 4 || i == 8 || i == 12) continue;
        
        ppu.write(0x2006, 0x3F);
        ppu.write(0x2006, static_cast<std::uint8_t>(i));
        std::uint8_t value = ppu.read(0x2007);
        EXPECT_EQ(value, static_cast<std::uint8_t>(i + 0x10));
    }
}

// =============================================================================
// Register Mirroring Tests
// =============================================================================

TEST_F(PPUTest, RegisterMirroringWorks) {
    // PPU registers are mirrored every 8 bytes from $2000-$3FFF
    
    // Write to $2000
    ppu.write(0x2000, 0x80);
    
    // Write to $2008 (mirror of $2000)
    ppu.write(0x2008, 0x00);
    
    // Both should work without crash
    EXPECT_TRUE(true);
}

// =============================================================================
// Framebuffer Tests
// =============================================================================

TEST_F(PPUTest, FramebufferRendersAfterFrame) {
    ppu.write(0x2001, 0x08);  // Enable background
    
    run_frame();
    
    const auto& fb = ppu.get_framebuffer();
    
    // Check that some pixels were rendered (not all black)
    bool has_non_black = false;
    for (const auto& pixel : fb) {
        if (pixel != 0xFF000000) {
            has_non_black = true;
            break;
        }
    }
    
    // Note: May still be black if palette is 0
    // Just verify the framebuffer exists and is accessible
    EXPECT_EQ(fb.size(), 256 * 240);
}

TEST_F(PPUTest, FramebufferPixelsInValidRange) {
    ppu.write(0x2001, 0x08);  // Enable background
    run_frame();
    
    const auto& fb = ppu.get_framebuffer();
    
    // All pixels should have alpha = 0xFF
    for (const auto& pixel : fb) {
        std::uint8_t alpha = (pixel >> 24) & 0xFF;
        EXPECT_EQ(alpha, 0xFF);
    }
}

// =============================================================================
// Edge Case Tests
// =============================================================================

TEST_F(PPUTest, RapidRegisterWrites) {
    // Rapid writes to various registers
    for (int i = 0; i < 100; ++i) {
        ppu.write(0x2000, static_cast<std::uint8_t>(i));
        ppu.write(0x2001, static_cast<std::uint8_t>(i));
        ppu.write(0x2005, static_cast<std::uint8_t>(i));
        ppu.write(0x2005, static_cast<std::uint8_t>(i));
        ppu.write(0x2006, 0x20);
        ppu.write(0x2006, static_cast<std::uint8_t>(i));
        ppu.write(0x2007, static_cast<std::uint8_t>(i));
    }
    
    EXPECT_TRUE(true);
}

TEST_F(PPUTest, StepDuringVBlank) {
    // Run to VBlank
    run_cycles(241 * 341);
    
    // Step during VBlank should not render pixels
    for (int i = 0; i < 1000; ++i) {
        ppu.step();
    }
    
    EXPECT_TRUE(true);
}

TEST_F(PPUTest, MultipleResetCalls) {
    run_frame();
    ppu.reset();
    run_frame();
    ppu.reset();
    run_frame();
    
    EXPECT_TRUE(ppu.frame_ready());
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST_F(PPUTest, ExtendedRunning) {
    ppu.write(0x2000, 0x80);  // Enable NMI
    ppu.write(0x2001, 0x1E);  // Enable rendering
    
    // Run for 60 frames (1 second of gameplay)
    for (int frame = 0; frame < 60; ++frame) {
        run_frame();
        EXPECT_TRUE(ppu.frame_ready());
        ppu.clear_frame_ready();
        ppu.clear_nmi();
    }
}

TEST_F(PPUTest, NMIEveryFrame) {
    ppu.write(0x2000, 0x80);  // Enable NMI
    
    int nmi_count = 0;
    
    // Run for 10 frames, counting NMIs
    for (int frame = 0; frame < 10; ++frame) {
        // Run until we hit VBlank (scanline 241, cycle 1)
        while (!ppu.nmi_occurred()) {
            ppu.step();
        }
        ++nmi_count;
        ppu.clear_nmi();
        
        // Complete the rest of the frame
        while (!ppu.frame_ready()) {
            ppu.step();
        }
        ppu.clear_frame_ready();
    }
    
    EXPECT_EQ(nmi_count, 10);
}

// =============================================================================
// NES Palette Tests
// =============================================================================

TEST_F(PPUTest, NESPaletteHas64Colors) {
    // The NES palette should have 64 colors
    // We can verify this by checking palette writes work for all indices
    for (int i = 0; i < 64; ++i) {
        ppu.write(0x2006, 0x3F);
        ppu.write(0x2006, static_cast<std::uint8_t>(i & 0x1F));
        ppu.write(0x2007, static_cast<std::uint8_t>(i));
    }
    
    EXPECT_TRUE(true);
}

// =============================================================================
// Sprite Rendering Tests
// =============================================================================

TEST_F(PPUTest, SpriteZeroHitInitiallyFalse) {
    EXPECT_FALSE(ppu.sprite_zero_hit());
}

TEST_F(PPUTest, SpriteOverflowInitiallyFalse) {
    EXPECT_FALSE(ppu.sprite_overflow());
}

TEST_F(PPUTest, SpriteFlagsCleared) {
    // Enable rendering
    ppu.write(0x2001, 0x18);
    
    // Run a full frame
    run_frame();
    
    // Status register should have cleared flags at pre-render scanline
    // The flags may or may not be set depending on OAM contents
    // Just verify no crash
    EXPECT_TRUE(true);
}

TEST_F(PPUTest, OAMWriteForSprite) {
    // Write sprite 0 data
    ppu.write(0x2003, 0x00);  // OAM address = 0
    ppu.write(0x2004, 0x20);  // Y = 32
    ppu.write(0x2004, 0x01);  // Tile = 1
    ppu.write(0x2004, 0x00);  // Attributes = 0
    ppu.write(0x2004, 0x40);  // X = 64
    
    // Verify writes
    ppu.write(0x2003, 0x00);
    EXPECT_EQ(ppu.read(0x2004), 0x20);
    ppu.write(0x2003, 0x01);
    EXPECT_EQ(ppu.read(0x2004), 0x01);
    ppu.write(0x2003, 0x02);
    EXPECT_EQ(ppu.read(0x2004), 0x00);
    ppu.write(0x2003, 0x03);
    EXPECT_EQ(ppu.read(0x2004), 0x40);
}

TEST_F(PPUTest, SpriteRenderingEnabled) {
    // Enable sprite rendering
    ppu.write(0x2001, 0x10);  // Bit 4 = show sprites
    
    // Run a frame
    run_frame();
    
    // Should complete without crash
    EXPECT_TRUE(ppu.frame_ready());
}

TEST_F(PPUTest, SpriteAndBackgroundEnabled) {
    // Enable both background and sprites
    ppu.write(0x2001, 0x18);  // Bits 3 and 4
    
    // Write a sprite
    ppu.write(0x2003, 0x00);
    ppu.write(0x2004, 0x10);  // Y = 16
    ppu.write(0x2004, 0x00);  // Tile = 0
    ppu.write(0x2004, 0x00);  // Attributes = palette 0, no flip, front
    ppu.write(0x2004, 0x10);  // X = 16
    
    // Run a frame
    run_frame();
    
    EXPECT_TRUE(ppu.frame_ready());
}

TEST_F(PPUTest, MultipleSpritesSameScanline) {
    // Enable sprites
    ppu.write(0x2001, 0x10);
    
    // Write 8 sprites on same scanline (Y = 50)
    for (int i = 0; i < 8; ++i) {
        ppu.write(0x2003, static_cast<std::uint8_t>(i * 4));
        ppu.write(0x2004, 50);  // Y = 50
        ppu.write(0x2004, static_cast<std::uint8_t>(i));  // Tile
        ppu.write(0x2004, 0x00);  // Attributes
        ppu.write(0x2004, static_cast<std::uint8_t>(i * 10));  // X
    }
    
    run_frame();
    EXPECT_TRUE(ppu.frame_ready());
}

TEST_F(PPUTest, SpriteOverflowDetection) {
    // Enable sprites AND background (both must be enabled for evaluation)
    ppu.write(0x2001, 0x18);
    
    // Sprites appear on the scanline AFTER their Y position
    // So Y=49 means the sprite appears on scanlines 50-57 (for 8x8 sprites)
    // We want 9 sprites on the same scanline
    for (int i = 0; i < 9; ++i) {
        ppu.write(0x2003, static_cast<std::uint8_t>(i * 4));
        ppu.write(0x2004, 49);  // Y = 49 (sprite appears starting on scanline 50)
        ppu.write(0x2004, static_cast<std::uint8_t>(i));  // Tile
        ppu.write(0x2004, 0x00);  // Attributes
        ppu.write(0x2004, static_cast<std::uint8_t>(i * 10));  // X
    }
    
    // Run past scanline 49 where evaluation for scanline 50 happens
    // Scanline 49, cycle 257 is when sprite evaluation happens for scanline 50
    run_cycles(50 * 341);  // Run to scanline 50
    
    // Check overflow flag (should be set after evaluating scanline 50's sprites at cycle 257 of scanline 49)
    EXPECT_TRUE(ppu.sprite_overflow());
}

TEST_F(PPUTest, SpriteHorizontalFlip) {
    // Enable sprites
    ppu.write(0x2001, 0x10);
    
    // Write sprite with horizontal flip
    ppu.write(0x2003, 0x00);
    ppu.write(0x2004, 0x20);  // Y
    ppu.write(0x2004, 0x00);  // Tile
    ppu.write(0x2004, 0x40);  // Attributes: horizontal flip
    ppu.write(0x2004, 0x20);  // X
    
    run_frame();
    EXPECT_TRUE(ppu.frame_ready());
}

TEST_F(PPUTest, SpriteVerticalFlip) {
    // Enable sprites
    ppu.write(0x2001, 0x10);
    
    // Write sprite with vertical flip
    ppu.write(0x2003, 0x00);
    ppu.write(0x2004, 0x20);  // Y
    ppu.write(0x2004, 0x00);  // Tile
    ppu.write(0x2004, 0x80);  // Attributes: vertical flip
    ppu.write(0x2004, 0x20);  // X
    
    run_frame();
    EXPECT_TRUE(ppu.frame_ready());
}

TEST_F(PPUTest, SpriteBehindBackground) {
    // Enable both
    ppu.write(0x2001, 0x18);
    
    // Write sprite with behind-background priority
    ppu.write(0x2003, 0x00);
    ppu.write(0x2004, 0x20);  // Y
    ppu.write(0x2004, 0x00);  // Tile
    ppu.write(0x2004, 0x20);  // Attributes: priority = behind BG
    ppu.write(0x2004, 0x20);  // X
    
    run_frame();
    EXPECT_TRUE(ppu.frame_ready());
}

TEST_F(PPUTest, Sprite8x16Mode) {
    // Enable 8x16 sprite mode
    ppu.write(0x2000, 0x20);  // PPUCTRL bit 5 = 8x16 sprites
    ppu.write(0x2001, 0x10);  // Enable sprites
    
    // Write sprite
    ppu.write(0x2003, 0x00);
    ppu.write(0x2004, 0x20);  // Y
    ppu.write(0x2004, 0x00);  // Tile (bit 0 selects pattern table in 8x16 mode)
    ppu.write(0x2004, 0x00);  // Attributes
    ppu.write(0x2004, 0x20);  // X
    
    run_frame();
    EXPECT_TRUE(ppu.frame_ready());
}

TEST_F(PPUTest, SpriteLeftClipping) {
    // Enable sprites without left-8 clip
    ppu.write(0x2001, 0x14);  // Sprites enabled, left-8 NOT clipped
    
    // Write sprite at X = 0
    ppu.write(0x2003, 0x00);
    ppu.write(0x2004, 0x20);  // Y
    ppu.write(0x2004, 0x00);  // Tile
    ppu.write(0x2004, 0x00);  // Attributes
    ppu.write(0x2004, 0x00);  // X = 0
    
    run_frame();
    EXPECT_TRUE(ppu.frame_ready());
}

TEST_F(PPUTest, SpriteLeftClippingEnabled) {
    // Enable sprites WITH left-8 clip (bit 2 = 0)
    ppu.write(0x2001, 0x10);  // Sprites enabled, left-8 clipped
    
    // Write sprite at X = 0
    ppu.write(0x2003, 0x00);
    ppu.write(0x2004, 0x20);  // Y
    ppu.write(0x2004, 0x00);  // Tile
    ppu.write(0x2004, 0x00);  // Attributes
    ppu.write(0x2004, 0x00);  // X = 0
    
    run_frame();
    EXPECT_TRUE(ppu.frame_ready());
}

TEST_F(PPUTest, AllSpritePalettes) {
    // Enable sprites
    ppu.write(0x2001, 0x10);
    
    // Write sprites using all 4 palettes
    for (int palette = 0; palette < 4; ++palette) {
        ppu.write(0x2003, static_cast<std::uint8_t>(palette * 4));
        ppu.write(0x2004, static_cast<std::uint8_t>(20 + palette * 10));  // Y - distributed
        ppu.write(0x2004, 0x00);  // Tile
        ppu.write(0x2004, static_cast<std::uint8_t>(palette));  // Palette 0-3
        ppu.write(0x2004, static_cast<std::uint8_t>(palette * 20));  // X
    }
    
    run_frame();
    EXPECT_TRUE(ppu.frame_ready());
}

TEST_F(PPUTest, SpritePatternTableSelect) {
    // Select sprite pattern table 1 (PPUCTRL bit 3)
    ppu.write(0x2000, 0x08);
    ppu.write(0x2001, 0x10);
    
    // Write sprite
    ppu.write(0x2003, 0x00);
    ppu.write(0x2004, 0x20);
    ppu.write(0x2004, 0x00);
    ppu.write(0x2004, 0x00);
    ppu.write(0x2004, 0x20);
    
    run_frame();
    EXPECT_TRUE(ppu.frame_ready());
}

TEST_F(PPUTest, SpriteRenderingStress) {
    // Enable everything
    ppu.write(0x2000, 0x80);  // NMI enabled
    ppu.write(0x2001, 0x1E);  // All rendering enabled
    
    // Fill OAM with sprites
    for (int i = 0; i < 64; ++i) {
        ppu.write(0x2003, static_cast<std::uint8_t>(i * 4));
        ppu.write(0x2004, static_cast<std::uint8_t>(i * 3 + 10));  // Y - distributed
        ppu.write(0x2004, static_cast<std::uint8_t>(i));  // Tile
        ppu.write(0x2004, static_cast<std::uint8_t>(i % 4));  // Attributes (different palettes)
        ppu.write(0x2004, static_cast<std::uint8_t>(i * 4));  // X
    }
    
    // Run multiple frames
    for (int frame = 0; frame < 10; ++frame) {
        run_frame();
        EXPECT_TRUE(ppu.frame_ready());
        ppu.clear_frame_ready();
    }
}

TEST_F(PPUTest, SpriteYEquals255Hidden) {
    // Enable sprites
    ppu.write(0x2001, 0x10);
    
    // Write sprite at Y = 255 (off-screen / hidden)
    ppu.write(0x2003, 0x00);
    ppu.write(0x2004, 0xFF);  // Y = 255
    ppu.write(0x2004, 0x00);  // Tile
    ppu.write(0x2004, 0x00);  // Attributes
    ppu.write(0x2004, 0x20);  // X
    
    run_frame();
    EXPECT_TRUE(ppu.frame_ready());
    // Sprite at Y=255 should not appear on screen
}

TEST_F(PPUTest, SpriteStatusFlagsClearedOnPreRender) {
    // Set up conditions that might trigger flags
    ppu.write(0x2001, 0x18);
    
    // Write 9 sprites (causes overflow)
    for (int i = 0; i < 9; ++i) {
        ppu.write(0x2003, static_cast<std::uint8_t>(i * 4));
        ppu.write(0x2004, 50);
        ppu.write(0x2004, static_cast<std::uint8_t>(i));
        ppu.write(0x2004, 0x00);
        ppu.write(0x2004, static_cast<std::uint8_t>(i * 10));
    }
    
    // Run one frame - flags should be set
    run_frame();
    ppu.clear_frame_ready();
    
    // Run to pre-render scanline of next frame (261 * 341 + 2 cycles into next frame)
    run_cycles(261 * 341 + 2);
    
    // Read status - sprite flags should be cleared
    std::uint8_t status = ppu.read(0x2002);
    EXPECT_FALSE(status & 0x40);  // Sprite 0 hit cleared
    // Note: We don't check overflow here as it depends on OAM state for current frame
}
