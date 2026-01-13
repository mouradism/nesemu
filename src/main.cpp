#define SDL_MAIN_HANDLED

#include "cpu.hpp"
#include "ppu.hpp"
#include "apu.hpp"
#include "memory.hpp"
#include "cartridge.hpp"
#include "video.hpp"
#include "audio.hpp"

#include <SDL.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

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
    
    // Load cartridge (ROM file optional for testing)
    Cartridge cart(argc >= 2 ? argv[1] : "");
    bool rom_loaded = (argc >= 2 && cart.loaded());
    
    if (argc >= 2 && !cart.loaded()) {
        std::cerr << "Warning: Failed to load ROM: " << argv[1] << '\n';
        std::cerr << "Continuing with test pattern...\n";
    }
    
    if (!rom_loaded) {
        std::cout << "No ROM loaded - using test tone and pattern.\n";
    }

    // Initialize emulation components
    Memory bus(cart);
    CPU cpu(bus);
    PPU ppu(bus);
    APU apu;

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

    std::uint64_t frame_counter = 0;
    Uint32 frame_start_time = 0;
    
    // Test tone state (used when no ROM loaded)
    double test_tone_phase = 0.0;
    std::vector<float> audio_samples;

    std::cout << "Entering main loop. Press ESC or close window to exit.\n";

    // Main emulation loop
    while (video.is_running()) {
        frame_start_time = SDL_GetTicks();

        // Process window events
        if (!video.handle_events()) {
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
        } else {
            // Use test pattern when no ROM loaded and PPU not ready
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
            std::cout << "Frame: " << frame_counter << "\n";
        }
    }

    // Cleanup
    std::cout << "Cleaning up...\n";
    SDL_Quit();
    
    std::cout << "Exit complete.\n";
    return 0;
}