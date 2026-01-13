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
    return chr_ram_flag_ ? chr_ram_ : chr_;
}

std::uint8_t Cartridge::read_prg(std::uint16_t addr) const {
    if (mapper_) {
        return mapper_->read_prg(addr);
    }
    return 0;
}

void Cartridge::write_prg(std::uint16_t addr, std::uint8_t value) {
    if (mapper_) {
        mapper_->write_prg(addr, value);
    }
}

std::uint8_t Cartridge::read_prg_ram(std::uint16_t addr) const {
    if (mapper_) {
        return mapper_->read_prg_ram(addr);
    }
    return 0;
}

void Cartridge::write_prg_ram(std::uint16_t addr, std::uint8_t value) {
    if (mapper_) {
        mapper_->write_prg_ram(addr, value);
    }
}

bool Cartridge::has_prg_ram() const {
    return mapper_ && mapper_->has_prg_ram();
}

std::uint8_t Cartridge::read_chr(std::uint16_t addr) const {
    if (mapper_) {
        return mapper_->read_chr(addr);
    }
    return 0;
}

void Cartridge::write_chr(std::uint16_t addr, std::uint8_t value) {
    if (mapper_) {
        mapper_->write_chr(addr, value);
    }
}

MirrorMode Cartridge::mirror_mode() const {
    if (mapper_) {
        return mapper_->mirror_mode();
    }
    return initial_mirror_;
}

bool Cartridge::irq_pending() const {
    return mapper_ && mapper_->irq_pending();
}

void Cartridge::clear_irq() {
    if (mapper_) {
        mapper_->clear_irq();
    }
}

void Cartridge::scanline_counter() {
    if (mapper_) {
        mapper_->scanline_counter();
    }
}

const char* Cartridge::mapper_name() const {
    if (mapper_) {
        return mapper_->mapper_name();
    }
    return "Unknown";
}

void Cartridge::reset() {
    if (mapper_) {
        mapper_->reset();
    }
}

bool Cartridge::load_from_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Cartridge: Failed to open file: " << path << '\n';
        return false;
    }
    
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
    // Check for iNES magic number
    if (data[0] != 'N' || data[1] != 'E' || data[2] != 'S' || data[3] != 0x1A) {
        std::cerr << "Cartridge: Invalid iNES header (bad magic number)\n";
        return false;
    }
    
    // Parse header
    prg_banks_ = data[4];
    chr_banks_ = data[5];
    
    std::uint8_t flags6 = data[6];
    std::uint8_t flags7 = data[7];
    
    // Mirroring
    if (flags6 & 0x08) {
        initial_mirror_ = MirrorMode::FourScreen;
    } else if (flags6 & 0x01) {
        initial_mirror_ = MirrorMode::Vertical;
    } else {
        initial_mirror_ = MirrorMode::Horizontal;
    }
    
    battery_ = (flags6 & 0x02) != 0;
    trainer_ = (flags6 & 0x04) != 0;
    mapper_id_ = ((flags6 >> 4) & 0x0F) | (flags7 & 0xF0);
    
    // Log info
    std::cout << "Cartridge loaded:\n";
    std::cout << "  PRG ROM: " << prg_banks_ << " x 16KB = " << (prg_banks_ * 16) << "KB\n";
    std::cout << "  CHR ROM: " << chr_banks_ << " x 8KB = " << (chr_banks_ * 8) << "KB\n";
    std::cout << "  Mapper: " << static_cast<int>(mapper_id_) << "\n";
    std::cout << "  Mirroring: ";
    switch (initial_mirror_) {
        case MirrorMode::Horizontal: std::cout << "Horizontal\n"; break;
        case MirrorMode::Vertical: std::cout << "Vertical\n"; break;
        case MirrorMode::FourScreen: std::cout << "Four-screen\n"; break;
        default: std::cout << "Unknown\n"; break;
    }
    std::cout << "  Battery: " << (battery_ ? "Yes" : "No") << "\n";
    std::cout << "  Trainer: " << (trainer_ ? "Yes" : "No") << "\n";
    
    // Calculate offsets
    std::size_t offset = 16;
    if (trainer_) {
        offset += 512;
    }
    
    // Extract PRG ROM
    std::size_t prg_size = prg_banks_ * 16384;
    if (offset + prg_size > data.size()) {
        std::cerr << "Cartridge: File too small for PRG ROM\n";
        return false;
    }
    
    prg_.resize(prg_size);
    std::copy(data.begin() + offset, data.begin() + offset + prg_size, prg_.begin());
    offset += prg_size;
    
    // Extract CHR ROM or allocate CHR RAM
    if (chr_banks_ > 0) {
        std::size_t chr_size = chr_banks_ * 8192;
        if (offset + chr_size > data.size()) {
            std::cerr << "Cartridge: File too small for CHR ROM\n";
            return false;
        }
        
        chr_.resize(chr_size);
        std::copy(data.begin() + offset, data.begin() + offset + chr_size, chr_.begin());
        chr_ram_flag_ = false;
    } else {
        chr_ram_.resize(8192, 0);
        chr_ram_flag_ = true;
        std::cout << "  CHR RAM: 8KB (no CHR ROM)\n";
    }
    
    // Create mapper
    mapper_ = create_mapper(mapper_id_, 
                            static_cast<std::uint8_t>(prg_banks_), 
                            static_cast<std::uint8_t>(chr_banks_),
                            initial_mirror_);
    
    if (mapper_) {
        // Give mapper access to ROM data
        mapper_->set_rom_data(&prg_, 
                              chr_ram_flag_ ? nullptr : &chr_,
                              chr_ram_flag_ ? &chr_ram_ : nullptr);
        
        std::cout << "  Mapper name: " << mapper_->mapper_name() << "\n";
    }
    
    return true;
}
