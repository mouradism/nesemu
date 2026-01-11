#include "ppu.hpp"
#include "memory.hpp"

PPU::PPU(Memory& bus) : bus_(bus) {}

void PPU::step() {
    // Placeholder PPU timing; in a real emulator, this would render and trigger NMI.
    ++cycle_;
    if (cycle_ >= 341) {
        cycle_ = 0;
        ++scanline_;
        if (scanline_ >= 262) {
            scanline_ = 0;
        }
    }
}
