#include "memory.hpp"
#include "cartridge.hpp"

Memory::Memory(Cartridge& cart) : cartridge_(cart) {}

std::uint8_t Memory::read(std::uint16_t addr) const {
    // Simple RAM mirroring for addresses under 0x2000
    if (addr < 0x2000) {
        return ram_[addr % ram_.size()];
    }
    // Cartridge or other hardware would be consulted here
    return 0;
}

void Memory::write(std::uint16_t addr, std::uint8_t value) {
    if (addr < 0x2000) {
        ram_[addr % ram_.size()] = value;
        return;
    }
    // Writes to other ranges would be routed to PPU/APU/cartridge in a full implementation
    (void)addr;
    (void)value;
}

Cartridge& Memory::cartridge() {
    return cartridge_;
}

const Cartridge& Memory::cartridge() const {
    return cartridge_;
}
