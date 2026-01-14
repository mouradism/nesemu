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
    
    // Reset sprite state
    sprite_count_ = 0;
    sprite_zero_hit_possible_ = false;
    sprite_zero_being_rendered_ = false;
    
    for (int i = 0; i < MAX_SPRITES_PER_SCANLINE; ++i) {
        secondary_oam_[i] = SpriteEntry{};
        sprite_shifter_pattern_lo_[i] = 0;
        sprite_shifter_pattern_hi_[i] = 0;
        sprite_x_counters_[i] = 0;
        sprite_attributes_[i] = 0;
        sprite_is_sprite0_[i] = false;
    }
    
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
            
            // Clear sprite shifters
            for (int i = 0; i < MAX_SPRITES_PER_SCANLINE; ++i) {
                sprite_shifter_pattern_lo_[i] = 0;
                sprite_shifter_pattern_hi_[i] = 0;
            }
        }
        
        // Copy vertical scroll bits from t to v at dots 280-304
        if (cycle_ >= 280 && cycle_ <= 304) {
            if (mask_ & 0x18) {
                transfer_address_y();
            }
        }
        
        // Odd frame cycle skip - skip cycle 340 on pre-render scanline
        // This effectively skips cycle 0 of the next frame
        if (cycle_ == 339 && odd_frame_ && (mask_ & 0x18)) {
            cycle_ = 340;  // Will become 341, then wrap to 0
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
                    bg_next_tile_id_ = ppu_read(0x2000 | (vram_addr_ & 0x0FFF));
                    break;
                case 2:
                    bg_next_tile_attrib_ = ppu_read(
                        0x23C0 | (vram_addr_ & 0x0C00) |
                        ((vram_addr_ >> 4) & 0x38) |
                        ((vram_addr_ >> 2) & 0x07)
                    );
                    if (vram_addr_ & 0x0002) bg_next_tile_attrib_ >>= 2;
                    if (vram_addr_ & 0x0040) bg_next_tile_attrib_ >>= 4;
                    bg_next_tile_attrib_ &= 0x03;
                    break;
                case 4: {
                    std::uint16_t pattern_addr = 
                        ((ctrl_ & 0x10) << 8) |
                        (static_cast<std::uint16_t>(bg_next_tile_id_) << 4) |
                        ((vram_addr_ >> 12) & 0x07);
                    bg_next_tile_lsb_ = ppu_read(pattern_addr);
                    break;
                }
                case 6: {
                    std::uint16_t pattern_addr = 
                        ((ctrl_ & 0x10) << 8) |
                        (static_cast<std::uint16_t>(bg_next_tile_id_) << 4) |
                        ((vram_addr_ >> 12) & 0x07) |
                        0x08;
                    bg_next_tile_msb_ = ppu_read(pattern_addr);
                    break;
                }
                case 7:
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
        
        // Sprite evaluation at cycle 257 (for next scanline)
        if (cycle_ == 257 && scanline_ < 240) {
            evaluate_sprites();
        }
        
        // Sprite pattern fetching (cycles 257-320)
        if (cycle_ == 321 && scanline_ < 240) {
            fetch_sprite_patterns();
        }
        
        // Unused nametable fetches at cycles 337-340
        // These are dummy fetches that some games rely on for timing
        if (cycle_ == 337 || cycle_ == 339) {
            bg_next_tile_id_ = ppu_read(0x2000 | (vram_addr_ & 0x0FFF));
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

void PPU::evaluate_sprites() {
    // Clear secondary OAM
    sprite_count_ = 0;
    sprite_zero_hit_possible_ = false;
    
    for (int i = 0; i < MAX_SPRITES_PER_SCANLINE; ++i) {
        secondary_oam_[i] = SpriteEntry{};
        sprite_is_sprite0_[i] = false;
    }
    
    // Only evaluate if rendering is enabled
    if (!(mask_ & 0x18)) {
        return;
    }
    
    int height = sprite_height();
    int next_scanline = scanline_ + 1;
    
    // Evaluate all 64 sprites in OAM for the NEXT scanline
    // NES sprite Y value: sprite appears on scanlines (Y+1) through (Y+height)
    for (int i = 0; i < 64; ++i) {
        std::uint8_t sprite_y = oam_[i * 4 + 0];
        
        // Check if sprite is on next scanline
        // diff should be 1..height for the sprite to be visible
        int diff = next_scanline - static_cast<int>(sprite_y);
        if (diff > 0 && diff <= height) {
            if (sprite_count_ < MAX_SPRITES_PER_SCANLINE) {
                // Copy sprite to secondary OAM
                secondary_oam_[sprite_count_].y = sprite_y;
                secondary_oam_[sprite_count_].tile_index = oam_[i * 4 + 1];
                secondary_oam_[sprite_count_].attributes = oam_[i * 4 + 2];
                secondary_oam_[sprite_count_].x = oam_[i * 4 + 3];
                secondary_oam_[sprite_count_].oam_index = static_cast<std::uint8_t>(i);
                
                // Track if this is sprite 0
                if (i == 0) {
                    sprite_zero_hit_possible_ = true;
                }
                
                sprite_count_++;
            } else {
                // More than 8 sprites on this scanline - set overflow flag
                status_ |= 0x20;
                break;  // Stop evaluating
            }
        }
    }
}

void PPU::fetch_sprite_patterns() {
    int height = sprite_height();
    int next_scanline = scanline_ + 1;
    
    for (int i = 0; i < sprite_count_; ++i) {
        SpriteEntry& sprite = secondary_oam_[i];
        
        std::uint8_t sprite_pattern_lo = 0;
        std::uint8_t sprite_pattern_hi = 0;
        std::uint16_t pattern_addr = 0;
        
        // Calculate row within sprite for NEXT scanline
        // Sprite appears at (sprite.y + 1), so row = next_scanline - (sprite.y + 1)
        int row = next_scanline - static_cast<int>(sprite.y) - 1;
        
        // Handle vertical flip
        if (sprite.attributes & 0x80) {
            row = (height - 1) - row;
        }
        
        if (height == 8) {
            // 8x8 sprites
            std::uint16_t pattern_table = (ctrl_ & 0x08) ? 0x1000 : 0x0000;
            pattern_addr = pattern_table | (sprite.tile_index << 4) | row;
        } else {
            // 8x16 sprites - tile index bit 0 selects pattern table
            std::uint16_t pattern_table = (sprite.tile_index & 0x01) ? 0x1000 : 0x0000;
            std::uint8_t tile = sprite.tile_index & 0xFE;
            
            if (row >= 8) {
                tile++;       // Bottom tile
                row -= 8;
            }
            
            pattern_addr = pattern_table | (tile << 4) | row;
        }
        
        // Fetch pattern data
        sprite_pattern_lo = ppu_read(pattern_addr);
        sprite_pattern_hi = ppu_read(pattern_addr + 8);
        
        // Handle horizontal flip
        if (sprite.attributes & 0x40) {
            // Flip bits in both bytes
            auto flip_byte = [](std::uint8_t b) {
                b = ((b & 0xF0) >> 4) | ((b & 0x0F) << 4);
                b = ((b & 0xCC) >> 2) | ((b & 0x33) << 2);
                b = ((b & 0xAA) >> 1) | ((b & 0x55) << 1);
                return b;
            };
            sprite_pattern_lo = flip_byte(sprite_pattern_lo);
            sprite_pattern_hi = flip_byte(sprite_pattern_hi);
        }
        
        // Store in shift registers
        sprite_shifter_pattern_lo_[i] = sprite_pattern_lo;
        sprite_shifter_pattern_hi_[i] = sprite_pattern_hi;
        sprite_x_counters_[i] = sprite.x;
        sprite_attributes_[i] = sprite.attributes;
        sprite_is_sprite0_[i] = (sprite.oam_index == 0);
    }
}

void PPU::render_sprite_pixel(std::uint8_t& sprite_pixel, std::uint8_t& sprite_palette, 
                               bool& sprite_priority, bool& is_sprite_zero) {
    sprite_pixel = 0;
    sprite_palette = 0;
    sprite_priority = false;
    is_sprite_zero = false;
    
    if (!(mask_ & 0x10)) {
        return;  // Sprites disabled
    }
    
    int x = cycle_ - 1;
    
    // Check left-8-pixel clip for sprites
    if (x < 8 && !(mask_ & 0x04)) {
        return;
    }
    
    // Find first non-transparent sprite pixel
    for (int i = 0; i < sprite_count_; ++i) {
        // Check if this sprite's X counter has reached 0 (sprite is active)
        int sprite_x = sprite_x_counters_[i];
        int offset = x - sprite_x;
        
        if (offset >= 0 && offset < 8) {
            // Get pixel from shift register
            std::uint8_t bit = 7 - offset;
            std::uint8_t pixel_lo = (sprite_shifter_pattern_lo_[i] >> bit) & 0x01;
            std::uint8_t pixel_hi = (sprite_shifter_pattern_hi_[i] >> bit) & 0x01;
            std::uint8_t pixel = (pixel_hi << 1) | pixel_lo;
            
            if (pixel != 0) {
                // Found a non-transparent pixel
                sprite_pixel = pixel;
                sprite_palette = (sprite_attributes_[i] & 0x03) + 4;  // Sprite palettes are 4-7
                sprite_priority = (sprite_attributes_[i] & 0x20) != 0;  // Behind background
                is_sprite_zero = sprite_is_sprite0_[i];
                return;  // First sprite wins (lowest OAM index has priority)
            }
        }
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
    
    // Get sprite pixel
    std::uint8_t sprite_pixel = 0;
    std::uint8_t sprite_palette = 0;
    bool sprite_priority = false;
    bool is_sprite_zero = false;
    
    render_sprite_pixel(sprite_pixel, sprite_palette, sprite_priority, is_sprite_zero);
    
    // Determine final pixel
    std::uint8_t final_pixel = 0;
    std::uint8_t final_palette = 0;
    
    if (bg_pixel == 0 && sprite_pixel == 0) {
        // Both transparent - use background color
        final_pixel = 0;
        final_palette = 0;
    } else if (bg_pixel == 0 && sprite_pixel != 0) {
        // Only sprite visible
        final_pixel = sprite_pixel;
        final_palette = sprite_palette;
    } else if (bg_pixel != 0 && sprite_pixel == 0) {
        // Only background visible
        final_pixel = bg_pixel;
        final_palette = bg_palette;
    } else {
        // Both visible - check priority
        if (sprite_priority) {
            // Sprite behind background
            final_pixel = bg_pixel;
            final_palette = bg_palette;
        } else {
            // Sprite in front of background
            final_pixel = sprite_pixel;
            final_palette = sprite_palette;
        }
        
        // Sprite 0 hit detection
        if (is_sprite_zero && sprite_zero_hit_possible_) {
            // Sprite 0 hit occurs when both BG and sprite are opaque
            // and both BG and sprite rendering are enabled
            if ((mask_ & 0x18) == 0x18) {
                // Don't trigger at x=255 or if left clipping affects it
                if (x < 255) {
                    if (x >= 8 || ((mask_ & 0x06) == 0x06)) {
                        status_ |= 0x40;  // Set sprite 0 hit
                    }
                }
            }
        }
    }
    
    // Get color from palette
    std::uint8_t palette_index = (final_palette << 2) | final_pixel;
    if (final_pixel == 0) {
        palette_index = 0;  // Transparent always uses palette index 0
    }
    
    std::uint8_t color_index = ppu_read(0x3F00 + palette_index) & 0x3F;
    std::uint32_t color = NES_PALETTE[color_index];
    
    framebuffer_[y * SCREEN_WIDTH + x] = color;
}

void PPU::load_background_shifters() {
    bg_shifter_pattern_lo_ = (bg_shifter_pattern_lo_ & 0xFF00) | bg_next_tile_lsb_;
    bg_shifter_pattern_hi_ = (bg_shifter_pattern_hi_ & 0xFF00) | bg_next_tile_msb_;
    
    bg_shifter_attrib_lo_ = (bg_shifter_attrib_lo_ & 0xFF00) | 
                            ((bg_next_tile_attrib_ & 0x01) ? 0xFF : 0x00);
    bg_shifter_attrib_hi_ = (bg_shifter_attrib_hi_ & 0xFF00) | 
                            ((bg_next_tile_attrib_ & 0x02) ? 0xFF : 0x00);
}

void PPU::update_shifters() {
    if (mask_ & 0x08) {
        bg_shifter_pattern_lo_ <<= 1;
        bg_shifter_pattern_hi_ <<= 1;
        bg_shifter_attrib_lo_ <<= 1;
        bg_shifter_attrib_hi_ <<= 1;
    }
}

void PPU::increment_scroll_x() {
    if ((vram_addr_ & 0x001F) == 31) {
        vram_addr_ &= ~0x001F;
        vram_addr_ ^= 0x0400;
    } else {
        vram_addr_++;
    }
}

void PPU::increment_scroll_y() {
    if ((vram_addr_ & 0x7000) != 0x7000) {
        vram_addr_ += 0x1000;
    } else {
        vram_addr_ &= ~0x7000;
        
        int y = (vram_addr_ & 0x03E0) >> 5;
        if (y == 29) {
            y = 0;
            vram_addr_ ^= 0x0800;
        } else if (y == 31) {
            y = 0;
        } else {
            y++;
        }
        vram_addr_ = (vram_addr_ & ~0x03E0) | (y << 5);
    }
}

void PPU::transfer_address_x() {
    if (mask_ & 0x18) {
        vram_addr_ = (vram_addr_ & ~0x041F) | (temp_addr_ & 0x041F);
    }
}

void PPU::transfer_address_y() {
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

std::uint8_t PPU::ppu_read(std::uint16_t addr) {
    addr &= 0x3FFF;
    
    if (addr < 0x2000) {
        return bus_.cartridge().read_chr(addr);
    } else if (addr < 0x3F00) {
        return vram_[mirror_nametable_addr(addr) & 0x07FF];
    } else {
        return read_palette(addr);
    }
}

void PPU::ppu_write(std::uint16_t addr, std::uint8_t value) {
    addr &= 0x3FFF;
    
    if (addr < 0x2000) {
        bus_.cartridge().write_chr(addr, value);
    } else if (addr < 0x3F00) {
        vram_[mirror_nametable_addr(addr) & 0x07FF] = value;
    } else {
        write_palette(addr, value);
    }
}

std::uint16_t PPU::mirror_nametable_addr(std::uint16_t addr) {
    addr &= 0x2FFF;
    
    Cartridge::MirrorMode mode = bus_.cartridge().mirror_mode();
    
    switch (mode) {
        case Cartridge::MirrorMode::Horizontal:
            if (addr >= 0x2800) {
                return (addr - 0x2800) + 0x0400;
            } else if (addr >= 0x2400) {
                return addr - 0x2400;
            }
            return addr - 0x2000;
            
        case Cartridge::MirrorMode::Vertical:
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

std::uint8_t PPU::read(std::uint16_t addr) {
    std::uint8_t result = 0;
    
    switch (addr & 0x0007) {
        case 0:
            break;
        case 1:
            break;
        case 2:
            result = (status_ & 0xE0);
            status_ &= ~0x80;
            write_latch_ = false;
            break;
        case 3:
            break;
        case 4:
            result = oam_[oam_addr_];
            break;
        case 5:
            break;
        case 6:
            break;
        case 7:
            result = vram_data_;
            vram_data_ = ppu_read(vram_addr_);
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
        case 0:
            ctrl_ = value;
            temp_addr_ = (temp_addr_ & 0xF3FF) | ((value & 0x03) << 10);
            break;
        case 1:
            mask_ = value;
            break;
        case 2:
            break;
        case 3:
            oam_addr_ = value;
            break;
        case 4:
            oam_[oam_addr_++] = value;
            break;
        case 5:
            if (!write_latch_) {
                fine_x_ = value & 0x07;
                temp_addr_ = (temp_addr_ & 0xFFE0) | (value >> 3);
            } else {
                temp_addr_ = (temp_addr_ & 0x8C1F) | 
                             ((value & 0x07) << 12) |
                             ((value & 0xF8) << 2);
            }
            write_latch_ = !write_latch_;
            break;
        case 6:
            if (!write_latch_) {
                temp_addr_ = (temp_addr_ & 0x00FF) | ((value & 0x3F) << 8);
            } else {
                temp_addr_ = (temp_addr_ & 0xFF00) | value;
                vram_addr_ = temp_addr_;
            }
            write_latch_ = !write_latch_;
            break;
        case 7:
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
