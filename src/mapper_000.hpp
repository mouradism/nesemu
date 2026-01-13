#pragma once

#include "mapper.hpp"

// Mapper 0: NROM
// - PRG ROM: 16KB or 32KB (no banking)
// - CHR ROM: 8KB (no banking)
// - No PRG RAM (typically)
class Mapper000 : public Mapper {
public:
    Mapper000(std::uint8_t prg_banks, std::uint8_t chr_banks)
        : Mapper(prg_banks, chr_banks) {}
    
    std::uint8_t read_prg(std::uint16_t addr) const override {
        if (!prg_rom_ || prg_rom_->empty()) return 0;
        
        addr &= 0x7FFF;  // Remove $8000 base
        
        // 16KB PRG ROM is mirrored
        if (prg_rom_->size() <= 0x4000) {
            addr &= 0x3FFF;
        }
        
        if (addr < prg_rom_->size()) {
            return (*prg_rom_)[addr];
        }
        return 0;
    }
    
    void write_prg(std::uint16_t addr, std::uint8_t value) override {
        // NROM has no writable PRG
        (void)addr;
        (void)value;
    }
    
    std::uint8_t read_chr(std::uint16_t addr) const override {
        addr &= 0x1FFF;
        
        if (chr_ram_ && !chr_ram_->empty()) {
            if (addr < chr_ram_->size()) {
                return (*chr_ram_)[addr];
            }
        } else if (chr_rom_ && !chr_rom_->empty()) {
            if (addr < chr_rom_->size()) {
                return (*chr_rom_)[addr];
            }
        }
        return 0;
    }
    
    void write_chr(std::uint16_t addr, std::uint8_t value) override {
        // Only write to CHR RAM
        if (chr_ram_ && !chr_ram_->empty()) {
            addr &= 0x1FFF;
            if (addr < chr_ram_->size()) {
                (*chr_ram_)[addr] = value;
            }
        }
    }
    
    std::uint8_t mapper_id() const override { return 0; }
    const char* mapper_name() const override { return "NROM"; }
};
