#pragma once

#include "mapper.hpp"

// Mapper 2: UxROM
// - PRG ROM: Up to 256KB (switchable 16KB bank at $8000, fixed last bank at $C000)
// - CHR RAM: 8KB (no CHR ROM banking)
// - No PRG RAM
class Mapper002 : public Mapper {
public:
    Mapper002(std::uint8_t prg_banks, std::uint8_t chr_banks)
        : Mapper(prg_banks, chr_banks) {
        reset();
    }
    
    void reset() override {
        prg_bank_ = 0;
    }
    
    std::uint8_t read_prg(std::uint16_t addr) const override {
        if (!prg_rom_ || prg_rom_->empty()) return 0;
        
        std::uint32_t mapped_addr = 0;
        
        if (addr >= 0x8000 && addr <= 0xBFFF) {
            // $8000-$BFFF: Switchable bank
            mapped_addr = (prg_bank_ * 0x4000) + (addr & 0x3FFF);
        } else if (addr >= 0xC000) {
            // $C000-$FFFF: Fixed to last bank
            std::uint8_t last_bank = prg_banks_ - 1;
            mapped_addr = (last_bank * 0x4000) + (addr & 0x3FFF);
        }
        
        if (mapped_addr < prg_rom_->size()) {
            return (*prg_rom_)[mapped_addr];
        }
        return 0;
    }
    
    void write_prg(std::uint16_t addr, std::uint8_t value) override {
        if (addr >= 0x8000) {
            // Any write to $8000-$FFFF selects bank
            prg_bank_ = value & 0x0F;
        }
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
        if (chr_ram_ && !chr_ram_->empty()) {
            addr &= 0x1FFF;
            if (addr < chr_ram_->size()) {
                (*chr_ram_)[addr] = value;
            }
        }
    }
    
    std::uint8_t mapper_id() const override { return 2; }
    const char* mapper_name() const override { return "UxROM"; }

private:
    std::uint8_t prg_bank_ = 0;
};
