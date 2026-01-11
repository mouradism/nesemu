#include "cpu.hpp"
#include "memory.hpp"

CPU::CPU(Memory& bus) : bus_(bus) {}

void CPU::reset() {
    cycles_ = 0;
}

int CPU::step() {
    // Placeholder CPU step; return a single cycle for now.
    ++cycles_;
    return 1;
}
