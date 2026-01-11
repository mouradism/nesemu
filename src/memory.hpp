#pragma once

#include <array>
#include <cstdint>

class Cartridge;

class Memory {
public:
    explicit Memory(Cartridge& cart);

    std::uint8_t read(std::uint16_t addr) const;
    void write(std::uint16_t addr, std::uint8_t value);

    Cartridge& cartridge();
    const Cartridge& cartridge() const;

private:
    Cartridge& cartridge_;
    std::array<std::uint8_t, 2048> ram_{};
};
