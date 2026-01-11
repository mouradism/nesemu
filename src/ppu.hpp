#pragma once

#include <cstdint>

class Memory;

class PPU {
public:
    explicit PPU(Memory& bus);

    void step();

private:
    Memory& bus_;
    std::uint32_t cycle_ = 0;
    std::uint32_t scanline_ = 0;
};
