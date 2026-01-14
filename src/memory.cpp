#include "memory.hpp"
#include "cartridge.hpp"
#include "ppu.hpp"
#include "apu.hpp"
#include "controller.hpp"

Memory::Memory(Cartridge& cart) : cartridge_(cart) {}

std::uint8_t Memory::read(std::uint16_t addr) {
    if (addr < 0x2000) {
        // Internal RAM (2KB, mirrored 4 times)
        return ram_[addr & 0x07FF];
    } 
    else if (addr < 0x4000) {
        // PPU registers ($2000-$2007, mirrored every 8 bytes)
        if (ppu_) {
            return ppu_->read(addr);
        }
        return 0;
    } 
    else if (addr < 0x4020) {
        // APU and I/O registers
        switch (addr) {
            case 0x4015:
                // APU Status
                if (apu_) {
                    return apu_->read(addr);
                }
                return 0;
                
            case 0x4016:
                // Controller 1
                if (controller1_) {
                    return controller1_->read();
                }
                return 0x40;  // Open bus
                
            case 0x4017:
                // Controller 2
                if (controller2_) {
                    return controller2_->read();
                }
                return 0x40;  // Open bus
                
            default:
                // Other APU registers are write-only
                return 0;
        }
    } 
    else if (addr < 0x6000) {
        // Expansion ROM (rarely used)
        return 0;
    } 
    else if (addr < 0x8000) {
        // PRG RAM ($6000-$7FFF)
        return cartridge_.read_prg_ram(addr);
    } 
    else {
        // PRG ROM ($8000-$FFFF)
        return cartridge_.read_prg(addr);
    }
}

void Memory::write(std::uint16_t addr, std::uint8_t value) {
    if (addr < 0x2000) {
        // Internal RAM (2KB, mirrored 4 times)
        ram_[addr & 0x07FF] = value;
    } 
    else if (addr < 0x4000) {
        // PPU registers ($2000-$2007, mirrored every 8 bytes)
        if (ppu_) {
            ppu_->write(addr, value);
        }
    } 
    else if (addr < 0x4020) {
        // APU and I/O registers
        switch (addr) {
            // Pulse 1 ($4000-$4003)
            case 0x4000:
            case 0x4001:
            case 0x4002:
            case 0x4003:
                if (apu_) {
                    apu_->write(addr, value);
                }
                break;
                
            // Pulse 2 ($4004-$4007)
            case 0x4004:
            case 0x4005:
            case 0x4006:
            case 0x4007:
                if (apu_) {
                    apu_->write(addr, value);
                }
                break;
                
            // Triangle ($4008-$400B)
            case 0x4008:
            case 0x4009:
            case 0x400A:
            case 0x400B:
                if (apu_) {
                    apu_->write(addr, value);
                }
                break;
                
            // Noise ($400C-$400F)
            case 0x400C:
            case 0x400D:
            case 0x400E:
            case 0x400F:
                if (apu_) {
                    apu_->write(addr, value);
                }
                break;
                
            // DMC ($4010-$4013)
            case 0x4010:
            case 0x4011:
            case 0x4012:
            case 0x4013:
                if (apu_) {
                    apu_->write(addr, value);
                }
                break;
                
            // OAM DMA ($4014)
            case 0x4014:
                dma_page_ = value;
                dma_pending_ = true;
                break;
                
            // APU Status ($4015)
            case 0x4015:
                if (apu_) {
                    apu_->write(addr, value);
                }
                break;
                
            // Controller 1 strobe ($4016)
            case 0x4016:
                if (controller1_) {
                    controller1_->write(value);
                }
                if (controller2_) {
                    controller2_->write(value);
                }
                break;
                
            // APU Frame Counter ($4017)
            case 0x4017:
                if (apu_) {
                    apu_->write(addr, value);
                }
                break;
                
            default:
                // Unused registers
                break;
        }
    } 
    else if (addr < 0x6000) {
        // Expansion ROM (rarely used, usually read-only)
    } 
    else if (addr < 0x8000) {
        // PRG RAM ($6000-$7FFF)
        cartridge_.write_prg_ram(addr, value);
    } 
    else {
        // PRG ROM ($8000-$FFFF) - writes may be mapper registers
        cartridge_.write_prg(addr, value);
    }
}

std::uint8_t Memory::peek(std::uint16_t addr) const {
    // Read without side effects (for debugging)
    if (addr < 0x2000) {
        return ram_[addr & 0x07FF];
    } 
    else if (addr < 0x4000) {
        // Can't peek PPU without side effects for some registers
        return 0;
    } 
    else if (addr < 0x4020) {
        return 0;
    } 
    else if (addr < 0x6000) {
        return 0;
    } 
    else if (addr < 0x8000) {
        return cartridge_.read_prg_ram(addr);
    } 
    else {
        return cartridge_.read_prg(addr);
    }
}

void Memory::execute_dma() {
    if (!dma_pending_ || !ppu_) {
        return;
    }
    
    // Transfer 256 bytes from CPU memory page to OAM
    std::uint16_t base_addr = static_cast<std::uint16_t>(dma_page_) << 8;
    
    for (int i = 0; i < 256; ++i) {
        std::uint8_t data = read(base_addr + i);
        
        // Write to OAM via OAMDATA ($2004)
        ppu_->write(0x2004, data);
    }
    
    dma_pending_ = false;
}

Cartridge& Memory::cartridge() {
    return cartridge_;
}

const Cartridge& Memory::cartridge() const {
    return cartridge_;
}
