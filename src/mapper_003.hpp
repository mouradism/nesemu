#pragma once

#include "mapper.hpp"

// Mapper 3: CNROM
// - PRG ROM: 16KB or 32KB (no banking, like NROM)
// - CHR ROM: Up to 32KB (switchable 8KB banks)
// - No PRG RAM
class Mapper003 : public Mapper {
public:
    Mapper003(std::uint8_t prg_banks, std::uint8_t chr_banks)
        : Mapper(prg_banks, chr_banks) {
        reset();
    }
    
    void reset() override {
        chr_bank_ = 0;
    }
    
    std::uint8_t read_prg(std::uint16_t addr) const override {
        if (!prg_rom_ || prg_rom_->empty()) return 0;
        
        addr &= 0x7FFF;
        
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
        if (addr >= 0x8000) {
            // Any write to $8000-$FFFF selects CHR bank
            chr_bank_ = value & 0x03;  // 2 bits = 4 banks max
        }
    }
    
    std::uint8_t read_chr(std::uint16_t addr) const override {
        addr &= 0x1FFF;
        std::uint32_t mapped_addr = (chr_bank_ * 0x2000) + addr;
        
        if (chr_ram_ && !chr_ram_->empty()) {
            mapped_addr &= (chr_ram_->size() - 1);
            return (*chr_ram_)[mapped_addr];
        } else if (chr_rom_ && !chr_rom_->empty()) {
            mapped_addr &= (chr_rom_->size() - 1);
            return (*chr_rom_)[mapped_addr];
        }
        return 0;
    }
    
    void write_chr(std::uint16_t addr, std::uint8_t value) override {
        if (chr_ram_ && !chr_ram_->empty()) {
            addr &= 0x1FFF;
            std::uint32_t mapped_addr = (chr_bank_ * 0x2000) + addr;
            mapped_addr &= (chr_ram_->size() - 1);
            (*chr_ram_)[mapped_addr] = value;
        }
    }
    
    std::uint8_t mapper_id() const override { return 3; }
    const char* mapper_name() const override { return "CNROM"; }

private:
    std::uint8_t chr_bank_ = 0;
};
