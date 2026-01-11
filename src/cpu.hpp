#pragma once

#include <cstdint>

class Memory;

class CPU {
public:
    explicit CPU(Memory& bus);

    void reset();
    int step(); // returns CPU cycles consumed

private:
    Memory& bus_;
    std::uint64_t cycles_ = 0;
};
