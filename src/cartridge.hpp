#pragma once

#include "mapper.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

class Cartridge {
public:
    // Re-export MirrorMode for backward compatibility
    using MirrorMode = ::MirrorMode;
    
    explicit Cartridge(const std::string& path);
    
    bool loaded() const;
    
    // PRG ROM access (CPU $8000-$FFFF)
    const std::vector<std::uint8_t>& prg() const;
    std::uint8_t read_prg(std::uint16_t addr) const;
    void write_prg(std::uint16_t addr, std::uint8_t value);
    
    // PRG RAM access (CPU $6000-$7FFF)
    std::uint8_t read_prg_ram(std::uint16_t addr) const;
    void write_prg_ram(std::uint16_t addr, std::uint8_t value);
    bool has_prg_ram() const;
    
    // CHR ROM/RAM access (PPU $0000-$1FFF)
    const std::vector<std::uint8_t>& chr() const;
    std::uint8_t read_chr(std::uint16_t addr) const;
    void write_chr(std::uint16_t addr, std::uint8_t value);
    
    // Mirroring (may be changed by mapper)
    MirrorMode mirror_mode() const;
    
    // IRQ support
    bool irq_pending() const;
    void clear_irq();
    void scanline_counter();
    
    // ROM info
    std::uint8_t mapper_id() const { return mapper_id_; }
    const char* mapper_name() const;
    std::size_t prg_banks() const { return prg_banks_; }
    std::size_t chr_banks() const { return chr_banks_; }
    bool has_chr_ram() const { return chr_ram_flag_; }
    bool has_battery() const { return battery_; }
    bool has_trainer() const { return trainer_; }
    
    // Reset mapper state
    void reset();

private:
    bool load_from_file(const std::string& path);
    bool parse_ines_header(const std::vector<std::uint8_t>& data);
    
    std::vector<std::uint8_t> prg_;      // PRG ROM data
    std::vector<std::uint8_t> chr_;      // CHR ROM data
    std::vector<std::uint8_t> chr_ram_;  // CHR RAM (if no CHR ROM)
    
    std::unique_ptr<Mapper> mapper_;
    
    bool loaded_ = false;
    bool chr_ram_flag_ = false;          // True if CHR is RAM, not ROM
    bool battery_ = false;               // Battery-backed PRG RAM
    bool trainer_ = false;               // 512-byte trainer present
    
    std::uint8_t mapper_id_ = 0;         // Mapper number
    std::size_t prg_banks_ = 0;          // Number of 16KB PRG banks
    std::size_t chr_banks_ = 0;          // Number of 8KB CHR banks
    
    MirrorMode initial_mirror_ = MirrorMode::Horizontal;
};
