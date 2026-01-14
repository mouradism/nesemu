#pragma once

#include <array>
#include <cstdint>
#include <functional>

class Cartridge;
class PPU;
class APU;
class Controller;

class Memory {
public:
    explicit Memory(Cartridge& cart);
    
    // Set optional component references
    void set_ppu(PPU* ppu) { ppu_ = ppu; }
    void set_apu(APU* apu) { apu_ = apu; }
    void set_controllers(Controller* ctrl1, Controller* ctrl2) { 
        controller1_ = ctrl1; 
        controller2_ = ctrl2; 
    }
    
    // Main read/write interface
    std::uint8_t read(std::uint16_t addr);
    void write(std::uint16_t addr, std::uint8_t value);
    
    // Const read (for debugging/testing without side effects)
    std::uint8_t peek(std::uint16_t addr) const;
    
    // Cartridge access
    Cartridge& cartridge();
    const Cartridge& cartridge() const;
    
    // OAM DMA
    bool dma_pending() const { return dma_pending_; }
    void clear_dma_pending() { dma_pending_ = false; }
    std::uint8_t dma_page() const { return dma_page_; }
    
    // DMA transfer callback (called by CPU during DMA)
    using DmaCallback = std::function<void(std::uint8_t)>;
    void set_dma_callback(DmaCallback callback) { dma_callback_ = callback; }
    void execute_dma();
    
    // Direct RAM access (for testing)
    std::uint8_t* ram_data() { return ram_.data(); }
    const std::uint8_t* ram_data() const { return ram_.data(); }

private:
    Cartridge& cartridge_;
    PPU* ppu_ = nullptr;
    APU* apu_ = nullptr;
    Controller* controller1_ = nullptr;
    Controller* controller2_ = nullptr;
    
    std::array<std::uint8_t, 2048> ram_{};
    
    // OAM DMA state
    bool dma_pending_ = false;
    std::uint8_t dma_page_ = 0;
    DmaCallback dma_callback_;
};
