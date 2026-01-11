#include "cartridge.hpp"

#include <fstream>

Cartridge::Cartridge(const std::string& path) {
    load_from_file(path);
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

void Cartridge::load_from_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        loaded_ = false;
        return;
    }

    // For now, just read the entire file into PRG for inspection.
    prg_.assign(std::istreambuf_iterator<char>(file), {});
    loaded_ = true;
}
