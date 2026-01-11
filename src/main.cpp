#define SDL_MAIN_HANDLED

#include "cpu.hpp"
#include "ppu.hpp"
#include "memory.hpp"
#include "cartridge.hpp"
#include "video.hpp"
#include "audio.hpp"

#include <SDL.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

int main(int argc, char** argv) {
    std::cout << "Starting nesemu...\n";
    std::cout << "Arguments: " << argc << "\n";
    
    // Load cartridge (ROM file optional for testing)
    Cartridge cart(argc >= 2 ? argv[1] : "");
    if (argc >= 2 && !cart.loaded()) {
        std::cerr << "Warning: Failed to load ROM: " << argv[1] << '\n';
        std::cerr << "Continuing with test pattern...\n";
    }

    // Initialize emulation components
    Memory bus(cart);
    CPU cpu(bus);
    PPU ppu(bus);

    // Initialize SDL library
    std::cout << "Initializing SDL...\n";
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
        return 3;
    }
    std::cout << "SDL initialized successfully.\n";

    // Initialize video subsystem (window, renderer, texture)
    Video video;
    if (!video.init(256, 240, 2)) {
        SDL_Quit();
        return 4;
    }

    // Initialize audio subsystem (audio device, sample queue)
    Audio audio;
    if (!audio.init(24000, 1)) {
        std::cerr << "Warning: Audio initialization failed, continuing without audio.\n";
    }

    // Allocate framebuffer for 256x240 NES resolution
    std::vector<std::uint32_t> framebuffer(256 * 240, 0);

    // Reset CPU to initial state
    cpu.reset();

    bool running = true;
    std::uint64_t frame_counter = 0;

    // Audio synthesis state for test tone (440 Hz sine wave)
    double phase = 0.0;
    const double tone_hz = 440.0;
    const double two_pi = 6.283185307179586;
    const double phase_inc = two_pi * tone_hz / 24000.0;
    const float amplitude = 0.1f;

    std::cout << "Entering main loop. Press ESC or close window to exit.\n";

    // Main emulation loop
    while (running && video.is_running()) {
        // Step CPU and PPU (PPU runs 3x faster than CPU)
        int cycles = cpu.step();
        for (int i = 0; i < cycles * 3; ++i) {
            ppu.step();
        }

        // Process window events (quit, ESC key)
        if (!video.handle_events()) {
            running = false;
        }

        // Generate and queue audio samples if audio device is initialized
        if (audio.is_initialized()) {
            const int target_samples = 480;  // ~10ms at 48kHz
            if (SDL_GetQueuedAudioSize(audio.is_initialized() ? 1 : 0) < static_cast<int>(target_samples * sizeof(float))) {
                std::vector<float> samples(512);
                // Generate sine wave for test tone
                for (int i = 0; i < 512; ++i) {
                    samples[i] = amplitude * static_cast<float>(std::sin(phase));
                    phase += phase_inc;
                    if (phase >= two_pi) {
                        phase -= two_pi;
                    }
                }
                audio.queue_samples(samples);
            }
        }

        // Update framebuffer with animated test pattern (temporary, will be replaced by PPU output)
        ++frame_counter;
        for (int y = 0; y < 240; ++y) {
            for (int x = 0; x < 256; ++x) {
                std::uint8_t r = static_cast<std::uint8_t>((x + frame_counter) & 0xFF);
                std::uint8_t g = static_cast<std::uint8_t>((y + frame_counter) & 0xFF);
                std::uint8_t b = static_cast<std::uint8_t>((x + y + frame_counter) & 0xFF);
                framebuffer[y * 256 + x] = (0xFFu << 24) | (r << 16) | (g << 8) | b;
            }
        }

        // Render current framebuffer to window
        video.update_frame(framebuffer);
        video.render();

        // Print frame counter for debug output
        if (frame_counter % 60 == 0) {
            std::cout << "Frame: " << frame_counter << "\n";
        }
    }

    // Cleanup SDL
    std::cout << "Cleaning up...\n";
    SDL_Quit();
    
    std::cout << "Exit complete.\n";
    return 0;
}