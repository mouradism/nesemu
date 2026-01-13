#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>

class Cartridge {
public:
    // Nametable mirroring modes
    enum class MirrorMode {
        Horizontal,   // Vertical arrangement (CIRAM A10 = PPU A11)
        Vertical,     // Horizontal arrangement (CIRAM A10 = PPU A10)
        SingleLower,  // Single screen, lower bank
        SingleUpper,  // Single screen, upper bank
        FourScreen    // Four-screen VRAM
    };
    
    explicit Cartridge(const std::string& path);
    
    bool loaded() const;
    
    // PRG ROM access (CPU $8000-$FFFF)
    const std::vector<std::uint8_t>& prg() const;
    std::uint8_t read_prg(std::uint16_t addr) const;
    void write_prg(std::uint16_t addr, std::uint8_t value);
    
    // CHR ROM/RAM access (PPU $0000-$1FFF)
    const std::vector<std::uint8_t>& chr() const;
    std::uint8_t read_chr(std::uint16_t addr) const;
    void write_chr(std::uint16_t addr, std::uint8_t value);
    
    // Mirroring
    MirrorMode mirror_mode() const { return mirror_mode_; }
    
    // ROM info
    std::uint8_t mapper_id() const { return mapper_id_; }
    std::size_t prg_banks() const { return prg_banks_; }
    std::size_t chr_banks() const { return chr_banks_; }
    bool has_chr_ram() const { return chr_ram_; }
    bool has_battery() const { return battery_; }
    bool has_trainer() const { return trainer_; }

private:
    bool load_from_file(const std::string& path);
    bool parse_ines_header(const std::vector<std::uint8_t>& data);
    
    std::vector<std::uint8_t> prg_;      // PRG ROM data
    std::vector<std::uint8_t> chr_;      // CHR ROM or RAM data
    
    bool loaded_ = false;
    bool chr_ram_ = false;               // True if CHR is RAM, not ROM
    bool battery_ = false;               // Battery-backed PRG RAM
    bool trainer_ = false;               // 512-byte trainer present
    
    std::uint8_t mapper_id_ = 0;         // Mapper number
    std::size_t prg_banks_ = 0;          // Number of 16KB PRG banks
    std::size_t chr_banks_ = 0;          // Number of 8KB CHR banks
    
    MirrorMode mirror_mode_ = MirrorMode::Horizontal;
};
