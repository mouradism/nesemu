#pragma once

#include <cstdint>
#include <vector>
#include <array>

class Memory;
class Cartridge;

class PPU {
public:
    static constexpr int SCREEN_WIDTH = 256;
    static constexpr int SCREEN_HEIGHT = 240;
    
    explicit PPU(Memory& bus);

    void step();
    void reset();
    
    // Framebuffer access
    bool frame_ready() const { return frame_complete_; }
    void clear_frame_ready() { frame_complete_ = false; }
    const std::vector<std::uint32_t>& get_framebuffer() const { return framebuffer_; }
    
    // Register access (memory-mapped I/O)
    std::uint8_t read(std::uint16_t addr);
    void write(std::uint16_t addr, std::uint8_t value);
    
    // NMI status
    bool nmi_occurred() const { return nmi_occurred_; }
    void clear_nmi() { nmi_occurred_ = false; }

private:
    Memory& bus_;
    
    // Timing
    std::uint32_t cycle_ = 0;
    std::uint32_t scanline_ = 0;
    bool frame_complete_ = false;
    bool nmi_occurred_ = false;
    bool odd_frame_ = false;
    
    // Framebuffer (ARGB8888 format)
    std::vector<std::uint32_t> framebuffer_;
    
    // PPU Registers
    std::uint8_t ctrl_ = 0;      // $2000 PPUCTRL
    std::uint8_t mask_ = 0;      // $2001 PPUMASK
    std::uint8_t status_ = 0;    // $2002 PPUSTATUS
    std::uint8_t oam_addr_ = 0;  // $2003 OAMADDR
    std::uint8_t scroll_x_ = 0;  // $2005 PPUSCROLL (X)
    std::uint8_t scroll_y_ = 0;  // $2005 PPUSCROLL (Y)
    std::uint16_t vram_addr_ = 0; // $2006 PPUADDR (v register)
    std::uint16_t temp_addr_ = 0; // Temporary VRAM address (t register)
    std::uint8_t fine_x_ = 0;     // Fine X scroll (3 bits)
    std::uint8_t vram_data_ = 0;  // $2007 PPUDATA buffer
    
    bool write_latch_ = false;   // Toggle for $2005/$2006
    
    // Internal VRAM (2KB nametables + palette)
    std::array<std::uint8_t, 2048> vram_{};      // Nametables
    std::array<std::uint8_t, 32> palette_{};     // Palette RAM
    std::array<std::uint8_t, 256> oam_{};        // Object Attribute Memory (sprites)
    
    // Background rendering shift registers
    std::uint16_t bg_shifter_pattern_lo_ = 0;
    std::uint16_t bg_shifter_pattern_hi_ = 0;
    std::uint16_t bg_shifter_attrib_lo_ = 0;
    std::uint16_t bg_shifter_attrib_hi_ = 0;
    
    // Background tile data latches
    std::uint8_t bg_next_tile_id_ = 0;
    std::uint8_t bg_next_tile_attrib_ = 0;
    std::uint8_t bg_next_tile_lsb_ = 0;
    std::uint8_t bg_next_tile_msb_ = 0;
    
    // NES color palette (64 colors, ARGB format)
    static const std::uint32_t NES_PALETTE[64];
    
    // Internal PPU memory access
    std::uint8_t ppu_read(std::uint16_t addr);
    void ppu_write(std::uint16_t addr, std::uint8_t value);
    
    // Rendering
    void render_pixel();
    void fetch_background_tile();
    void load_background_shifters();
    void update_shifters();
    
    // Scrolling helpers
    void increment_scroll_x();
    void increment_scroll_y();
    void transfer_address_x();
    void transfer_address_y();
    
    // VBlank
    void set_vblank();
    void clear_vblank();
    
    // Palette
    std::uint8_t read_palette(std::uint16_t addr);
    void write_palette(std::uint16_t addr, std::uint8_t value);
    
    // Nametable mirroring
    std::uint16_t mirror_nametable_addr(std::uint16_t addr);
};
