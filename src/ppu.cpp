#include "ppu.hpp"
#include "memory.hpp"
#include "cartridge.hpp"

// NES color palette (ARGB format) - standard NES palette
const std::uint32_t PPU::NES_PALETTE[64] = {
    0xFF666666, 0xFF002A88, 0xFF1412A7, 0xFF3B00A4, 0xFF5C007E, 0xFF6E0040, 0xFF6C0600, 0xFF561D00,
    0xFF333500, 0xFF0B4800, 0xFF005200, 0xFF004F08, 0xFF00404D, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFADADAD, 0xFF155FD9, 0xFF4240FF, 0xFF7527FE, 0xFFA01ACC, 0xFFB71E7B, 0xFFB53120, 0xFF994E00,
    0xFF6B6D00, 0xFF388700, 0xFF0C9300, 0xFF008F32, 0xFF007C8D, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFFFFEFF, 0xFF64B0FF, 0xFF9290FF, 0xFFC676FF, 0xFFF36AFF, 0xFFFE6ECC, 0xFFFE8170, 0xFFEA9E22,
    0xFFBCBE00, 0xFF88D800, 0xFF5CE430, 0xFF45E082, 0xFF48CDDE, 0xFF4F4F4F, 0xFF000000, 0xFF000000,
    0xFFFFFEFF, 0xFFC0DFFF, 0xFFD3D2FF, 0xFFE8C8FF, 0xFFFBC2FF, 0xFFFEC4EA, 0xFFFECCC5, 0xFFF7D8A5,
    0xFFE4E594, 0xFFCFEF96, 0xFFBDF4AB, 0xFFB3F3CC, 0xFFB5EBF2, 0xFFB8B8B8, 0xFF000000, 0xFF000000
};

PPU::PPU(Memory& bus) 
    : bus_(bus),
      framebuffer_(SCREEN_WIDTH * SCREEN_HEIGHT, 0xFF000000) {
}

void PPU::reset() {
    cycle_ = 0;
    scanline_ = 0;
    frame_complete_ = false;
    nmi_occurred_ = false;
    odd_frame_ = false;
    
    ctrl_ = 0;
    mask_ = 0;
    status_ = 0;
    oam_addr_ = 0;
    vram_addr_ = 0;
    temp_addr_ = 0;
    fine_x_ = 0;
    vram_data_ = 0;
    write_latch_ = false;
    
    bg_shifter_pattern_lo_ = 0;
    bg_shifter_pattern_hi_ = 0;
    bg_shifter_attrib_lo_ = 0;
    bg_shifter_attrib_hi_ = 0;
    
    bg_next_tile_id_ = 0;
    bg_next_tile_attrib_ = 0;
    bg_next_tile_lsb_ = 0;
    bg_next_tile_msb_ = 0;
    
    std::fill(framebuffer_.begin(), framebuffer_.end(), 0xFF000000);
    std::fill(vram_.begin(), vram_.end(), 0);
    std::fill(palette_.begin(), palette_.end(), 0);
    std::fill(oam_.begin(), oam_.end(), 0);
}

void PPU::step() {
    // Pre-render scanline (-1 or 261)
    if (scanline_ == 261) {
        if (cycle_ == 1) {
            // Clear VBlank, sprite 0 hit, and sprite overflow
            clear_vblank();
            status_ &= ~0x60;
        }
        
        // Copy vertical scroll bits from t to v at dots 280-304
        if (cycle_ >= 280 && cycle_ <= 304) {
            if (mask_ & 0x18) {  // If rendering enabled
                transfer_address_y();
            }
        }
    }
    
    // Visible scanlines (0-239) and pre-render scanline
    if (scanline_ < 240 || scanline_ == 261) {
        // Background tile fetching (cycles 1-256 and 321-336)
        if ((cycle_ >= 1 && cycle_ <= 256) || (cycle_ >= 321 && cycle_ <= 336)) {
            update_shifters();
            
            switch ((cycle_ - 1) % 8) {
                case 0:
                    load_background_shifters();
                    // Fetch nametable byte
                    bg_next_tile_id_ = ppu_read(0x2000 | (vram_addr_ & 0x0FFF));
                    break;
                case 2:
                    // Fetch attribute table byte
                    bg_next_tile_attrib_ = ppu_read(
                        0x23C0 | (vram_addr_ & 0x0C00) |
                        ((vram_addr_ >> 4) & 0x38) |
                        ((vram_addr_ >> 2) & 0x07)
                    );
                    // Select correct 2-bit palette from attribute byte
                    if (vram_addr_ & 0x0002) bg_next_tile_attrib_ >>= 2;
                    if (vram_addr_ & 0x0040) bg_next_tile_attrib_ >>= 4;
                    bg_next_tile_attrib_ &= 0x03;
                    break;
                case 4: {
                    // Fetch pattern table tile low byte
                    std::uint16_t pattern_addr = 
                        ((ctrl_ & 0x10) << 8) |  // Background pattern table select
                        (static_cast<std::uint16_t>(bg_next_tile_id_) << 4) |
                        ((vram_addr_ >> 12) & 0x07);  // Fine Y scroll
                    bg_next_tile_lsb_ = ppu_read(pattern_addr);
                    break;
                }
                case 6: {
                    // Fetch pattern table tile high byte
                    std::uint16_t pattern_addr = 
                        ((ctrl_ & 0x10) << 8) |
                        (static_cast<std::uint16_t>(bg_next_tile_id_) << 4) |
                        ((vram_addr_ >> 12) & 0x07) |
                        0x08;  // High plane offset
                    bg_next_tile_msb_ = ppu_read(pattern_addr);
                    break;
                }
                case 7:
                    // Increment horizontal scroll
                    if (mask_ & 0x18) {
                        increment_scroll_x();
                    }
                    break;
            }
        }
        
        // Increment vertical scroll at cycle 256
        if (cycle_ == 256 && (mask_ & 0x18)) {
            increment_scroll_y();
        }
        
        // Copy horizontal scroll bits from t to v at cycle 257
        if (cycle_ == 257 && (mask_ & 0x18)) {
            load_background_shifters();
            transfer_address_x();
        }
    }
    
    // Visible scanlines: render pixels
    if (scanline_ < 240 && cycle_ >= 1 && cycle_ <= 256) {
        render_pixel();
    }
    
    // VBlank begins at scanline 241, cycle 1
    if (scanline_ == 241 && cycle_ == 1) {
        set_vblank();
    }
    
    // Advance cycle and scanline
    ++cycle_;
    if (cycle_ >= 341) {
        cycle_ = 0;
        ++scanline_;
        
        if (scanline_ >= 262) {
            scanline_ = 0;
            frame_complete_ = true;
            odd_frame_ = !odd_frame_;
        }
    }
}

std::uint8_t PPU::ppu_read(std::uint16_t addr) {
    addr &= 0x3FFF;
    
    if (addr < 0x2000) {
        // Pattern tables - read from cartridge CHR ROM/RAM
        return bus_.cartridge().read_chr(addr);
    } else if (addr < 0x3F00) {
        // Nametables
        return vram_[mirror_nametable_addr(addr) & 0x07FF];
    } else {
        // Palette
        return read_palette(addr);
    }
}

void PPU::ppu_write(std::uint16_t addr, std::uint8_t value) {
    addr &= 0x3FFF;
    
    if (addr < 0x2000) {
        // Pattern tables - write to cartridge CHR RAM
        bus_.cartridge().write_chr(addr, value);
    } else if (addr < 0x3F00) {
        // Nametables
        vram_[mirror_nametable_addr(addr) & 0x07FF] = value;
    } else {
        // Palette
        write_palette(addr, value);
    }
}

std::uint16_t PPU::mirror_nametable_addr(std::uint16_t addr) {
    addr &= 0x2FFF;  // Mirror $3000-$3EFF to $2000-$2EFF
    
    Cartridge::MirrorMode mode = bus_.cartridge().mirror_mode();
    
    switch (mode) {
        case Cartridge::MirrorMode::Horizontal:
            // $2000/$2400 -> $2000, $2800/$2C00 -> $2400
            if (addr >= 0x2800) {
                return (addr - 0x2800) + 0x0400;
            } else if (addr >= 0x2400) {
                return addr - 0x2400;
            }
            return addr - 0x2000;
            
        case Cartridge::MirrorMode::Vertical:
            // $2000/$2800 -> $2000, $2400/$2C00 -> $2400
            return (addr & 0x07FF);
            
        case Cartridge::MirrorMode::SingleLower:
            return addr & 0x03FF;
            
        case Cartridge::MirrorMode::SingleUpper:
            return (addr & 0x03FF) + 0x0400;
            
        case Cartridge::MirrorMode::FourScreen:
        default:
            return addr & 0x0FFF;
    }
}

void PPU::render_pixel() {
    int x = cycle_ - 1;
    int y = scanline_;
    
    if (x < 0 || x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) {
        return;
    }
    
    std::uint8_t bg_pixel = 0;
    std::uint8_t bg_palette = 0;
    
    // Get background pixel if rendering enabled
    if (mask_ & 0x08) {
        // Check left-8-pixel clip
        if (x >= 8 || (mask_ & 0x02)) {
            std::uint16_t bit_mux = 0x8000 >> fine_x_;
            
            std::uint8_t pixel_lo = (bg_shifter_pattern_lo_ & bit_mux) ? 1 : 0;
            std::uint8_t pixel_hi = (bg_shifter_pattern_hi_ & bit_mux) ? 1 : 0;
            bg_pixel = (pixel_hi << 1) | pixel_lo;
            
            std::uint8_t palette_lo = (bg_shifter_attrib_lo_ & bit_mux) ? 1 : 0;
            std::uint8_t palette_hi = (bg_shifter_attrib_hi_ & bit_mux) ? 1 : 0;
            bg_palette = (palette_hi << 1) | palette_lo;
        }
    }
    
    // Get color from palette
    std::uint8_t palette_index = 0;
    if (bg_pixel != 0) {
        palette_index = (bg_palette << 2) | bg_pixel;
    }
    
    std::uint8_t color_index = ppu_read(0x3F00 + palette_index) & 0x3F;
    std::uint32_t color = NES_PALETTE[color_index];
    
    framebuffer_[y * SCREEN_WIDTH + x] = color;
}

void PPU::load_background_shifters() {
    // Load next tile data into shift registers
    bg_shifter_pattern_lo_ = (bg_shifter_pattern_lo_ & 0xFF00) | bg_next_tile_lsb_;
    bg_shifter_pattern_hi_ = (bg_shifter_pattern_hi_ & 0xFF00) | bg_next_tile_msb_;
    
    // Expand 2-bit palette to fill 8 bits
    bg_shifter_attrib_lo_ = (bg_shifter_attrib_lo_ & 0xFF00) | 
                            ((bg_next_tile_attrib_ & 0x01) ? 0xFF : 0x00);
    bg_shifter_attrib_hi_ = (bg_shifter_attrib_hi_ & 0xFF00) | 
                            ((bg_next_tile_attrib_ & 0x02) ? 0xFF : 0x00);
}

void PPU::update_shifters() {
    if (mask_ & 0x08) {  // Background rendering enabled
        bg_shifter_pattern_lo_ <<= 1;
        bg_shifter_pattern_hi_ <<= 1;
        bg_shifter_attrib_lo_ <<= 1;
        bg_shifter_attrib_hi_ <<= 1;
    }
}

void PPU::increment_scroll_x() {
    // Increment coarse X, wrapping at 32 and toggling nametable
    if ((vram_addr_ & 0x001F) == 31) {
        vram_addr_ &= ~0x001F;       // Clear coarse X
        vram_addr_ ^= 0x0400;        // Toggle horizontal nametable
    } else {
        vram_addr_++;
    }
}

void PPU::increment_scroll_y() {
    // Increment fine Y
    if ((vram_addr_ & 0x7000) != 0x7000) {
        vram_addr_ += 0x1000;
    } else {
        vram_addr_ &= ~0x7000;  // Clear fine Y
        
        // Increment coarse Y
        int y = (vram_addr_ & 0x03E0) >> 5;
        if (y == 29) {
            y = 0;
            vram_addr_ ^= 0x0800;  // Toggle vertical nametable
        } else if (y == 31) {
            y = 0;  // Don't toggle nametable (attribute table wrap)
        } else {
            y++;
        }
        vram_addr_ = (vram_addr_ & ~0x03E0) | (y << 5);
    }
}

void PPU::transfer_address_x() {
    // Copy horizontal bits from t to v
    // v: ....A.. ...BCDEF <- t: ....A.. ...BCDEF
    if (mask_ & 0x18) {
        vram_addr_ = (vram_addr_ & ~0x041F) | (temp_addr_ & 0x041F);
    }
}

void PPU::transfer_address_y() {
    // Copy vertical bits from t to v
    // v: GHIA.BC DEF..... <- t: GHIA.BC DEF.....
    if (mask_ & 0x18) {
        vram_addr_ = (vram_addr_ & ~0x7BE0) | (temp_addr_ & 0x7BE0);
    }
}

void PPU::set_vblank() {
    status_ |= 0x80;
    
    if (ctrl_ & 0x80) {
        nmi_occurred_ = true;
    }
}

void PPU::clear_vblank() {
    status_ &= ~0x80;
    nmi_occurred_ = false;
}

std::uint8_t PPU::read(std::uint16_t addr) {
    std::uint8_t result = 0;
    
    switch (addr & 0x0007) {
        case 0: // $2000 PPUCTRL - write only
            break;
            
        case 1: // $2001 PPUMASK - write only
            break;
            
        case 2: // $2002 PPUSTATUS
            result = (status_ & 0xE0);
            status_ &= ~0x80;
            write_latch_ = false;
            break;
            
        case 3: // $2003 OAMADDR - write only
            break;
            
        case 4: // $2004 OAMDATA
            result = oam_[oam_addr_];
            break;
            
        case 5: // $2005 PPUSCROLL - write only
            break;
            
        case 6: // $2006 PPUADDR - write only
            break;
            
        case 7: // $2007 PPUDATA
            result = vram_data_;
            vram_data_ = ppu_read(vram_addr_);
            
            // Palette reads are not delayed
            if ((vram_addr_ & 0x3FFF) >= 0x3F00) {
                result = vram_data_;
            }
            
            vram_addr_ += (ctrl_ & 0x04) ? 32 : 1;
            break;
    }
    
    return result;
}

void PPU::write(std::uint16_t addr, std::uint8_t value) {
    switch (addr & 0x0007) {
        case 0: // $2000 PPUCTRL
            ctrl_ = value;
            // t: ...GH.. ........ <- d: ......GH
            temp_addr_ = (temp_addr_ & 0xF3FF) | ((value & 0x03) << 10);
            break;
            
        case 1: // $2001 PPUMASK
            mask_ = value;
            break;
            
        case 2: // $2002 PPUSTATUS - read only
            break;
            
        case 3: // $2003 OAMADDR
            oam_addr_ = value;
            break;
            
        case 4: // $2004 OAMDATA
            oam_[oam_addr_++] = value;
            break;
            
        case 5: // $2005 PPUSCROLL
            if (!write_latch_) {
                // First write: X scroll
                fine_x_ = value & 0x07;
                temp_addr_ = (temp_addr_ & 0xFFE0) | (value >> 3);
            } else {
                // Second write: Y scroll
                temp_addr_ = (temp_addr_ & 0x8C1F) | 
                             ((value & 0x07) << 12) |
                             ((value & 0xF8) << 2);
            }
            write_latch_ = !write_latch_;
            break;
            
        case 6: // $2006 PPUADDR
            if (!write_latch_) {
                // First write: high byte
                temp_addr_ = (temp_addr_ & 0x00FF) | ((value & 0x3F) << 8);
            } else {
                // Second write: low byte
                temp_addr_ = (temp_addr_ & 0xFF00) | value;
                vram_addr_ = temp_addr_;
            }
            write_latch_ = !write_latch_;
            break;
            
        case 7: // $2007 PPUDATA
            ppu_write(vram_addr_, value);
            vram_addr_ += (ctrl_ & 0x04) ? 32 : 1;
            break;
    }
}

std::uint8_t PPU::read_palette(std::uint16_t addr) {
    addr &= 0x1F;
    if ((addr & 0x13) == 0x10) {
        addr &= ~0x10;
    }
    return palette_[addr];
}

void PPU::write_palette(std::uint16_t addr, std::uint8_t value) {
    addr &= 0x1F;
    if ((addr & 0x13) == 0x10) {
        addr &= ~0x10;
    }
    palette_[addr] = value;
}
