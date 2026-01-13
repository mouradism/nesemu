#include "cartridge.hpp"

#include <fstream>
#include <iostream>
#include <algorithm>

Cartridge::Cartridge(const std::string& path) {
    if (!path.empty()) {
        loaded_ = load_from_file(path);
    }
}

bool Cartridge::loaded() const {
    return loaded_;
}

const std::vector<std::uint8_t>& Cartridge::prg() const {
    return prg_;
}

const std::vector<std::uint8_t>& Cartridge::chr() const {
    return chr_;
}

std::uint8_t Cartridge::read_prg(std::uint16_t addr) const {
    if (prg_.empty()) {
        return 0;
    }
    
    // Simple mapping for NROM (mapper 0)
    // $8000-$BFFF: First 16KB of PRG ROM
    // $C000-$FFFF: Last 16KB of PRG ROM (or mirror of first if only 16KB)
    addr &= 0x7FFF;  // Remove $8000 offset
    
    if (prg_.size() <= 0x4000) {
        // 16KB PRG ROM - mirror
        addr &= 0x3FFF;
    }
    
    if (addr < prg_.size()) {
        return prg_[addr];
    }
    
    return 0;
}

void Cartridge::write_prg(std::uint16_t addr, std::uint8_t value) {
    // NROM has no PRG RAM, writes are ignored
    // Other mappers would handle bank switching here
    (void)addr;
    (void)value;
}

std::uint8_t Cartridge::read_chr(std::uint16_t addr) const {
    if (chr_.empty()) {
        return 0;
    }
    
    // CHR is 8KB ($0000-$1FFF)
    addr &= 0x1FFF;
    
    if (addr < chr_.size()) {
        return chr_[addr];
    }
    
    return 0;
}

void Cartridge::write_chr(std::uint16_t addr, std::uint8_t value) {
    // Only write if CHR is RAM
    if (chr_ram_ && !chr_.empty()) {
        addr &= 0x1FFF;
        if (addr < chr_.size()) {
            chr_[addr] = value;
        }
    }
}

bool Cartridge::load_from_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Cartridge: Failed to open file: " << path << '\n';
        return false;
    }
    
    // Read entire file into buffer
    std::vector<std::uint8_t> data{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    };
    
    if (data.size() < 16) {
        std::cerr << "Cartridge: File too small (< 16 bytes)\n";
        return false;
    }
    
    return parse_ines_header(data);
}

bool Cartridge::parse_ines_header(const std::vector<std::uint8_t>& data) {
    // Check for iNES magic number: "NES" followed by MS-DOS EOF
    if (data[0] != 'N' || data[1] != 'E' || data[2] != 'S' || data[3] != 0x1A) {
        std::cerr << "Cartridge: Invalid iNES header (bad magic number)\n";
        return false;
    }
    
    // Parse header
    prg_banks_ = data[4];  // Number of 16KB PRG ROM banks
    chr_banks_ = data[5];  // Number of 8KB CHR ROM banks
    
    std::uint8_t flags6 = data[6];
    std::uint8_t flags7 = data[7];
    
    // Mirroring
    if (flags6 & 0x08) {
        mirror_mode_ = MirrorMode::FourScreen;
    } else if (flags6 & 0x01) {
        mirror_mode_ = MirrorMode::Vertical;
    } else {
        mirror_mode_ = MirrorMode::Horizontal;
    }
    
    // Battery-backed PRG RAM
    battery_ = (flags6 & 0x02) != 0;
    
    // 512-byte trainer
    trainer_ = (flags6 & 0x04) != 0;
    
    // Mapper number
    mapper_id_ = ((flags6 >> 4) & 0x0F) | (flags7 & 0xF0);
    
    // Log cartridge info
    std::cout << "Cartridge loaded:\n";
    std::cout << "  PRG ROM: " << prg_banks_ << " x 16KB = " << (prg_banks_ * 16) << "KB\n";
    std::cout << "  CHR ROM: " << chr_banks_ << " x 8KB = " << (chr_banks_ * 8) << "KB\n";
    std::cout << "  Mapper: " << static_cast<int>(mapper_id_) << "\n";
    std::cout << "  Mirroring: ";
    switch (mirror_mode_) {
        case MirrorMode::Horizontal: std::cout << "Horizontal\n"; break;
        case MirrorMode::Vertical: std::cout << "Vertical\n"; break;
        case MirrorMode::FourScreen: std::cout << "Four-screen\n"; break;
        default: std::cout << "Unknown\n"; break;
    }
    std::cout << "  Battery: " << (battery_ ? "Yes" : "No") << "\n";
    std::cout << "  Trainer: " << (trainer_ ? "Yes" : "No") << "\n";
    
    // Calculate data offsets
    std::size_t offset = 16;  // Header is 16 bytes
    
    if (trainer_) {
        offset += 512;  // Skip trainer
    }
    
    // Extract PRG ROM
    std::size_t prg_size = prg_banks_ * 16384;  // 16KB per bank
    if (offset + prg_size > data.size()) {
        std::cerr << "Cartridge: File too small for PRG ROM\n";
        return false;
    }
    
    prg_.resize(prg_size);
    std::copy(data.begin() + offset, data.begin() + offset + prg_size, prg_.begin());
    offset += prg_size;
    
    // Extract CHR ROM or allocate CHR RAM
    if (chr_banks_ > 0) {
        std::size_t chr_size = chr_banks_ * 8192;  // 8KB per bank
        if (offset + chr_size > data.size()) {
            std::cerr << "Cartridge: File too small for CHR ROM\n";
            return false;
        }
        
        chr_.resize(chr_size);
        std::copy(data.begin() + offset, data.begin() + offset + chr_size, chr_.begin());
        chr_ram_ = false;
    } else {
        // No CHR ROM means CHR RAM (8KB)
        chr_.resize(8192, 0);
        chr_ram_ = true;
        std::cout << "  CHR RAM: 8KB (no CHR ROM)\n";
    }
    
    return true;
}
