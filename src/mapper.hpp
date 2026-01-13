#pragma once

#include <cstdint>
#include <vector>
#include <memory>

// Forward declaration
class Cartridge;

// Nametable mirroring modes
enum class MirrorMode {
    Horizontal,   // Vertical arrangement (CIRAM A10 = PPU A11)
    Vertical,     // Horizontal arrangement (CIRAM A10 = PPU A10)
    SingleLower,  // Single screen, lower bank
    SingleUpper,  // Single screen, upper bank
    FourScreen    // Four-screen VRAM
};

// Abstract base class for all mappers
class Mapper {
public:
    Mapper(std::uint8_t prg_banks, std::uint8_t chr_banks)
        : prg_banks_(prg_banks), chr_banks_(chr_banks) {}
    
    virtual ~Mapper() = default;
    
    // PRG ROM access ($8000-$FFFF)
    virtual std::uint8_t read_prg(std::uint16_t addr) const = 0;
    virtual void write_prg(std::uint16_t addr, std::uint8_t value) = 0;
    
    // CHR ROM/RAM access ($0000-$1FFF)
    virtual std::uint8_t read_chr(std::uint16_t addr) const = 0;
    virtual void write_chr(std::uint16_t addr, std::uint8_t value) = 0;
    
    // PRG RAM access ($6000-$7FFF) - optional
    virtual std::uint8_t read_prg_ram(std::uint16_t addr) const { (void)addr; return 0; }
    virtual void write_prg_ram(std::uint16_t addr, std::uint8_t value) { (void)addr; (void)value; }
    virtual bool has_prg_ram() const { return false; }
    
    // Mirroring mode (can be changed by mapper)
    virtual MirrorMode mirror_mode() const { return mirror_mode_; }
    void set_mirror_mode(MirrorMode mode) { mirror_mode_ = mode; }
    
    // IRQ support (for mappers like MMC3)
    virtual bool irq_pending() const { return false; }
    virtual void clear_irq() {}
    virtual void scanline_counter() {}
    
    // Reset mapper state
    virtual void reset() {}
    
    // Mapper identification
    virtual std::uint8_t mapper_id() const = 0;
    virtual const char* mapper_name() const = 0;
    
    // Set ROM data pointers (called by Cartridge after loading)
    void set_rom_data(const std::vector<std::uint8_t>* prg, 
                      const std::vector<std::uint8_t>* chr,
                      std::vector<std::uint8_t>* chr_ram) {
        prg_rom_ = prg;
        chr_rom_ = chr;
        chr_ram_ = chr_ram;
    }

protected:
    std::uint8_t prg_banks_;
    std::uint8_t chr_banks_;
    MirrorMode mirror_mode_ = MirrorMode::Horizontal;
    
    // ROM data pointers
    const std::vector<std::uint8_t>* prg_rom_ = nullptr;
    const std::vector<std::uint8_t>* chr_rom_ = nullptr;
    std::vector<std::uint8_t>* chr_ram_ = nullptr;
    
    friend class Cartridge;
};

// Factory function to create mapper by ID
std::unique_ptr<Mapper> create_mapper(std::uint8_t mapper_id, 
                                       std::uint8_t prg_banks, 
                                       std::uint8_t chr_banks,
                                       MirrorMode initial_mirror);
