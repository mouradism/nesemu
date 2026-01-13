#pragma once

#include "mapper.hpp"
#include <array>

// Mapper 1: MMC1 (Nintendo MMC1)
// - PRG ROM: Up to 512KB (switchable 16KB/32KB banks)
// - CHR ROM/RAM: Up to 128KB (switchable 4KB/8KB banks)
// - PRG RAM: 8KB at $6000-$7FFF (optional, battery-backed)
// - Mirroring: Switchable
class Mapper001 : public Mapper {
public:
    Mapper001(std::uint8_t prg_banks, std::uint8_t chr_banks)
        : Mapper(prg_banks, chr_banks) {
        reset();
    }
    
    void reset() override {
        shift_register_ = 0x10;  // Reset value
        write_count_ = 0;
        
        // Control register defaults
        control_ = 0x0C;  // PRG ROM mode 3 (fix last bank), CHR ROM mode 0
        chr_bank_0_ = 0;
        chr_bank_1_ = 0;
        prg_bank_ = 0;
        
        prg_ram_.fill(0);
        prg_ram_enabled_ = true;
    }
    
    std::uint8_t read_prg(std::uint16_t addr) const override {
        if (!prg_rom_ || prg_rom_->empty()) return 0;
        
        std::uint32_t mapped_addr = 0;
        
        if (addr >= 0x8000 && addr <= 0xBFFF) {
            // $8000-$BFFF: Switchable or fixed bank
            std::uint8_t prg_mode = (control_ >> 2) & 0x03;
            
            if (prg_mode == 0 || prg_mode == 1) {
                // 32KB mode: ignore low bit of bank number
                std::uint8_t bank = (prg_bank_ & 0x0E);
                mapped_addr = (bank * 0x4000) + (addr & 0x3FFF);
            } else if (prg_mode == 2) {
                // Fix first bank at $8000, switch $C000
                mapped_addr = addr & 0x3FFF;
            } else {
                // prg_mode == 3: Switch $8000, fix last bank at $C000
                mapped_addr = ((prg_bank_ & 0x0F) * 0x4000) + (addr & 0x3FFF);
            }
        } else if (addr >= 0xC000) {
            // $C000-$FFFF: Switchable or fixed bank
            std::uint8_t prg_mode = (control_ >> 2) & 0x03;
            
            if (prg_mode == 0 || prg_mode == 1) {
                // 32KB mode
                std::uint8_t bank = (prg_bank_ & 0x0E) | 0x01;
                mapped_addr = (bank * 0x4000) + (addr & 0x3FFF);
            } else if (prg_mode == 2) {
                // Switch $C000
                mapped_addr = ((prg_bank_ & 0x0F) * 0x4000) + (addr & 0x3FFF);
            } else {
                // prg_mode == 3: Fix last bank
                std::uint8_t last_bank = prg_banks_ - 1;
                mapped_addr = (last_bank * 0x4000) + (addr & 0x3FFF);
            }
        }
        
        if (mapped_addr < prg_rom_->size()) {
            return (*prg_rom_)[mapped_addr];
        }
        return 0;
    }
    
    void write_prg(std::uint16_t addr, std::uint8_t value) override {
        if (addr < 0x8000) return;
        
        // Reset shift register if bit 7 set
        if (value & 0x80) {
            shift_register_ = 0x10;
            write_count_ = 0;
            control_ |= 0x0C;  // Set PRG ROM mode to 3
            return;
        }
        
        // Shift in bit 0
        shift_register_ = ((shift_register_ >> 1) | ((value & 0x01) << 4)) & 0x1F;
        write_count_++;
        
        if (write_count_ == 5) {
            // Register is full, write to appropriate register
            std::uint8_t reg = (addr >> 13) & 0x03;
            
            switch (reg) {
                case 0:  // $8000-$9FFF: Control
                    control_ = shift_register_;
                    update_mirroring();
                    break;
                case 1:  // $A000-$BFFF: CHR bank 0
                    chr_bank_0_ = shift_register_;
                    break;
                case 2:  // $C000-$DFFF: CHR bank 1
                    chr_bank_1_ = shift_register_;
                    break;
                case 3:  // $E000-$FFFF: PRG bank
                    prg_bank_ = shift_register_ & 0x0F;
                    prg_ram_enabled_ = !(shift_register_ & 0x10);
                    break;
            }
            
            shift_register_ = 0x10;
            write_count_ = 0;
        }
    }
    
    std::uint8_t read_chr(std::uint16_t addr) const override {
        addr &= 0x1FFF;
        std::uint32_t mapped_addr = 0;
        
        bool chr_mode = (control_ & 0x10) != 0;  // 0 = 8KB, 1 = 4KB
        
        if (!chr_mode) {
            // 8KB mode
            std::uint8_t bank = chr_bank_0_ & 0x1E;  // Ignore low bit
            mapped_addr = (bank * 0x1000) + addr;
        } else {
            // 4KB mode
            if (addr < 0x1000) {
                mapped_addr = (chr_bank_0_ * 0x1000) + addr;
            } else {
                mapped_addr = (chr_bank_1_ * 0x1000) + (addr & 0x0FFF);
            }
        }
        
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
        if (!chr_ram_ || chr_ram_->empty()) return;
        
        addr &= 0x1FFF;
        std::uint32_t mapped_addr = 0;
        
        bool chr_mode = (control_ & 0x10) != 0;
        
        if (!chr_mode) {
            std::uint8_t bank = chr_bank_0_ & 0x1E;
            mapped_addr = (bank * 0x1000) + addr;
        } else {
            if (addr < 0x1000) {
                mapped_addr = (chr_bank_0_ * 0x1000) + addr;
            } else {
                mapped_addr = (chr_bank_1_ * 0x1000) + (addr & 0x0FFF);
            }
        }
        
        mapped_addr &= (chr_ram_->size() - 1);
        (*chr_ram_)[mapped_addr] = value;
    }
    
    std::uint8_t read_prg_ram(std::uint16_t addr) const override {
        if (!prg_ram_enabled_) return 0;
        return prg_ram_[addr & 0x1FFF];
    }
    
    void write_prg_ram(std::uint16_t addr, std::uint8_t value) override {
        if (!prg_ram_enabled_) return;
        prg_ram_[addr & 0x1FFF] = value;
    }
    
    bool has_prg_ram() const override { return true; }
    
    std::uint8_t mapper_id() const override { return 1; }
    const char* mapper_name() const override { return "MMC1"; }

private:
    void update_mirroring() {
        switch (control_ & 0x03) {
            case 0: mirror_mode_ = MirrorMode::SingleLower; break;
            case 1: mirror_mode_ = MirrorMode::SingleUpper; break;
            case 2: mirror_mode_ = MirrorMode::Vertical; break;
            case 3: mirror_mode_ = MirrorMode::Horizontal; break;
        }
    }
    
    // Shift register
    std::uint8_t shift_register_ = 0x10;
    std::uint8_t write_count_ = 0;
    
    // Registers
    std::uint8_t control_ = 0x0C;
    std::uint8_t chr_bank_0_ = 0;
    std::uint8_t chr_bank_1_ = 0;
    std::uint8_t prg_bank_ = 0;
    
    // PRG RAM
    std::array<std::uint8_t, 0x2000> prg_ram_{};
    bool prg_ram_enabled_ = true;
};
