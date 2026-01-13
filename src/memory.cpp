#include "memory.hpp"
#include "cartridge.hpp"

Memory::Memory(Cartridge& cart) : cartridge_(cart) {}

std::uint8_t Memory::read(std::uint16_t addr) const {
    if (addr < 0x2000) {
        // Internal RAM (2KB, mirrored 4 times)
        return ram_[addr & 0x07FF];
    } else if (addr < 0x4000) {
        // PPU registers ($2000-$2007, mirrored every 8 bytes)
        // TODO: Route to PPU
        return 0;
    } else if (addr < 0x4020) {
        // APU and I/O registers
        // TODO: Route to APU/Input
        return 0;
    } else if (addr < 0x6000) {
        // Expansion ROM (rarely used)
        return 0;
    } else if (addr < 0x8000) {
        // PRG RAM ($6000-$7FFF)
        return cartridge_.read_prg_ram(addr);
    } else {
        // PRG ROM ($8000-$FFFF)
        return cartridge_.read_prg(addr);
    }
}

void Memory::write(std::uint16_t addr, std::uint8_t value) {
    if (addr < 0x2000) {
        // Internal RAM (2KB, mirrored 4 times)
        ram_[addr & 0x07FF] = value;
    } else if (addr < 0x4000) {
        // PPU registers ($2000-$2007, mirrored every 8 bytes)
        // TODO: Route to PPU
    } else if (addr < 0x4020) {
        // APU and I/O registers
        // TODO: Route to APU/Input
    } else if (addr < 0x6000) {
        // Expansion ROM (rarely used)
    } else if (addr < 0x8000) {
        // PRG RAM ($6000-$7FFF)
        cartridge_.write_prg_ram(addr, value);
    } else {
        // PRG ROM ($8000-$FFFF) - writes may be mapper registers
        cartridge_.write_prg(addr, value);
    }
}

Cartridge& Memory::cartridge() {
    return cartridge_;
}

const Cartridge& Memory::cartridge() const {
    return cartridge_;
}
