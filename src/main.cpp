#define SDL_MAIN_HANDLED

#include "cpu.hpp"
#include "ppu.hpp"
#include "apu.hpp"
#include "memory.hpp"
#include "cartridge.hpp"
#include "controller.hpp"
#include "input.hpp"
#include "video.hpp"
#include "audio.hpp"

#include <SDL.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>
#include <fstream>

// NES timing constants
namespace NESConfig {
    constexpr int SCREEN_WIDTH = 256;
    constexpr int SCREEN_HEIGHT = 240;
    constexpr int WINDOW_SCALE = 2;
    constexpr int SAMPLE_RATE = 44100;
    constexpr int CPU_CYCLES_PER_FRAME = 29780;  // ~1.79MHz / 60fps
    constexpr int TARGET_FPS = 60;
    constexpr int FRAME_TIME_MS = 1000 / TARGET_FPS;
    constexpr int SAMPLES_PER_FRAME = SAMPLE_RATE / TARGET_FPS;  // ~735 samples
}

// Check if file exists
bool file_exists(const std::string& filename) {
    std::ifstream file(filename);
    return file.good();
}

// Generate test tone when no ROM is loaded
void generate_test_tone(std::vector<float>& samples, double& phase) {
    constexpr double TONE_HZ = 440.0;
    constexpr double TWO_PI = 6.283185307179586;
    constexpr double PHASE_INC = TWO_PI * TONE_HZ / NESConfig::SAMPLE_RATE;
    constexpr float AMPLITUDE = 0.1f;
    
    samples.resize(NESConfig::SAMPLES_PER_FRAME);
    for (size_t i = 0; i < samples.size(); ++i) {
        samples[i] = AMPLITUDE * static_cast<float>(std::sin(phase));
        phase += PHASE_INC;
        if (phase >= TWO_PI) {
            phase -= TWO_PI;
        }
    }
}

// Generate test pattern when PPU frame is not ready
void generate_test_pattern(std::vector<std::uint32_t>& framebuffer, std::uint64_t frame_counter) {
    for (int y = 0; y < NESConfig::SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < NESConfig::SCREEN_WIDTH; ++x) {
            std::uint8_t r = static_cast<std::uint8_t>((x + frame_counter) & 0xFF);
            std::uint8_t g = static_cast<std::uint8_t>((y + frame_counter) & 0xFF);
            std::uint8_t b = static_cast<std::uint8_t>((x + y + frame_counter) & 0xFF);
            framebuffer[y * NESConfig::SCREEN_WIDTH + x] = (0xFFu << 24) | (r << 16) | (g << 8) | b;
        }
    }
}

int main(int argc, char** argv) {
    std::cout << "Starting nesemu...\n";
    std::cout << Input::get_default_mapping() << "\n\n";
    
    // Determine which ROM to load
    std::string rom_path;
    bool rom_loaded = false;
    
    // Priority 1: Check command line argument
    if (argc >= 2) {
        rom_path = argv[1];
        std::cout << "Loading ROM from command line: " << rom_path << "\n";
    }
    // Priority 2: Check for Super_Mario_Bros.nes in current directory
    else if (file_exists("Super_Mario_Bros.nes")) {
        rom_path = "Super_Mario_Bros.nes";
        std::cout << "Found Super_Mario_Bros.nes in current directory.\n";
    }
    // Priority 3: No ROM found, use test mode
    else {
        std::cout << "No ROM found. Checked:\n";
        std::cout << "  - Command line arguments\n";
        std::cout << "  - Super_Mario_Bros.nes in current directory\n";
        std::cout << "Using test tone and pattern...\n";
    }
    
    // Load cartridge
    Cartridge cart(rom_path);
    rom_loaded = cart.loaded();
    
    if (!rom_path.empty() && !rom_loaded) {
        std::cerr << "Warning: Failed to load ROM: " << rom_path << '\n';
        std::cerr << "Continuing with test pattern...\n";
    }
    
    if (rom_loaded) {
        std::cout << "? ROM loaded successfully!\n";
        std::cout << "  PRG ROM: " << (cart.prg_banks() * 16) << " KB\n";
        std::cout << "  CHR ROM: " << (cart.chr_banks() * 8) << " KB\n";
        std::cout << "  Mapper: " << static_cast<int>(cart.mapper_id()) << " (" << cart.mapper_name() << ")\n";
    }

    // Initialize emulation components
    Memory bus(cart);
    CPU cpu(bus);
    PPU ppu(bus);
    APU apu;
    
    // Initialize controllers
    Controller controller1;
    Controller controller2;
    
    // Wire controllers to memory bus
    bus.set_ppu(&ppu);
    bus.set_apu(&apu);
    bus.set_controllers(&controller1, &controller2);
    
    // Initialize input handler
    Input input;
    input.set_controllers(&controller1, &controller2);

    // Initialize SDL library
    std::cout << "Initializing SDL...\n";
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
        return 1;
    }
    std::cout << "SDL initialized successfully.\n";

    // Initialize video subsystem
    Video video;
    if (!video.init(NESConfig::SCREEN_WIDTH, NESConfig::SCREEN_HEIGHT, NESConfig::WINDOW_SCALE)) {
        SDL_Quit();
        return 2;
    }

    // Initialize audio subsystem
    Audio audio;
    if (!audio.init(NESConfig::SAMPLE_RATE, 1)) {
        std::cerr << "Warning: Audio initialization failed, continuing without audio.\n";
    }

    // Allocate framebuffer for NES resolution
    std::vector<std::uint32_t> framebuffer(NESConfig::SCREEN_WIDTH * NESConfig::SCREEN_HEIGHT, 0);

    // Reset emulation components
    cpu.reset();
    ppu.reset();
    apu.reset();
    controller1.reset();
    controller2.reset();

    std::uint64_t frame_counter = 0;
    Uint32 frame_start_time = 0;
    
    // Test tone state (used when no ROM loaded)
    double test_tone_phase = 0.0;
    std::vector<float> audio_samples;

    std::cout << "Entering main loop. Press ESC or close window to exit.\n";

    // Main emulation loop
    while (video.is_running()) {
        frame_start_time = SDL_GetTicks();

        // Process window events and input
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                video.quit();
                break;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                video.quit();
                break;
            }
            
            // Process controller input events
            input.process_event(event);
        }
        
        // Also update from keyboard state (for held keys)
        input.update_from_keyboard_state();
        
        if (!video.is_running()) {
            break;
        }

        // Emulate one frame worth of CPU/PPU/APU cycles
        int cycles_this_frame = 0;
        while (cycles_this_frame < NESConfig::CPU_CYCLES_PER_FRAME) {
            int cpu_cycles = cpu.step();
            cycles_this_frame += cpu_cycles;

            // PPU runs 3x faster than CPU
            for (int i = 0; i < cpu_cycles * 3; ++i) {
                ppu.step();
            }

            // APU runs at CPU speed
            for (int i = 0; i < cpu_cycles; ++i) {
                apu.clock();
            }
            
            // Handle NMI from PPU
            if (ppu.nmi_occurred()) {
                ppu.clear_nmi();
                // TODO: Trigger NMI on CPU when CPU supports it
                // cpu.trigger_nmi();
            }
            
            // Handle OAM DMA
            if (bus.dma_pending()) {
                bus.execute_dma();
                // DMA takes 513-514 CPU cycles
                cycles_this_frame += 514;
            }
        }

        // Queue audio samples
        if (audio.is_initialized()) {
            if (rom_loaded) {
                // Use APU output for real ROM
                audio_samples = apu.get_samples();
            } else {
                // Use test tone when no ROM loaded
                generate_test_tone(audio_samples, test_tone_phase);
            }
            
            if (!audio_samples.empty()) {
                audio.queue_samples(audio_samples);
            }
        }

        // Update framebuffer
        ++frame_counter;
        if (rom_loaded && ppu.frame_ready()) {
            // Use real PPU output
            framebuffer = ppu.get_framebuffer();
            ppu.clear_frame_ready();
        } else if (!rom_loaded) {
            // Use test pattern when no ROM loaded
            generate_test_pattern(framebuffer, frame_counter);
        }

        // Render frame
        video.update_frame(framebuffer);
        video.render();

        // Frame timing - maintain 60 FPS
        Uint32 frame_time = SDL_GetTicks() - frame_start_time;
        if (frame_time < NESConfig::FRAME_TIME_MS) {
            SDL_Delay(NESConfig::FRAME_TIME_MS - frame_time);
        }

        // Debug output every second
        if (frame_counter % NESConfig::TARGET_FPS == 0) {
            std::cout << "Frame: " << frame_counter;
            if (controller1.get_state() != 0) {
                std::cout << " | P1: 0x" << std::hex << static_cast<int>(controller1.get_state()) << std::dec;
            }
            std::cout << "\n";
        }
    }

    // Cleanup
    std::cout << "Cleaning up...\n";
    SDL_Quit();
    
    std::cout << "Exit complete.\n";
    return 0;
}