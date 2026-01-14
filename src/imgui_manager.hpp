#pragma once

#include <SDL.h>
#include <cstdint>

class CPU;
class PPU;
class APU;
class Audio;
class Controller;

// Manages ImGui initialization, rendering, and debug UI
class ImGuiManager {
public:
    ImGuiManager();
    ~ImGuiManager();

    // Initialize ImGui with SDL2 window and OpenGL context
    bool init(SDL_Window* window, SDL_GLContext gl_context);

    // Start a new ImGui frame
    void new_frame();

    // Render all ImGui content
    void render();

    // Handle SDL events for ImGui input
    void process_event(const SDL_Event& event);

    // Set references to emulator components for debugging
    void set_emulator_refs(CPU* cpu, PPU* ppu, APU* apu, Audio* audio,
                          Controller* controller1, Controller* controller2);

    // Show/hide the debug window
    void toggle_debug_window() { show_debug_window_ = !show_debug_window_; }
    bool is_debug_window_visible() const { return show_debug_window_; }

    // Check if ImGui wants to capture mouse/keyboard input
    bool wants_mouse() const;
    bool wants_keyboard() const;

private:
    SDL_Window* window_ = nullptr;
    SDL_GLContext gl_context_ = nullptr;
    bool initialized_ = false;
    bool show_debug_window_ = true;

    // Emulator references
    CPU* cpu_ = nullptr;
    PPU* ppu_ = nullptr;
    APU* apu_ = nullptr;
    Audio* audio_ = nullptr;
    Controller* controller1_ = nullptr;
    Controller* controller2_ = nullptr;

    // UI state
    float fps_ = 0.0f;
    int frame_count_ = 0;
    float frame_time_ms_ = 0.0f;

    // Debug UI functions
    void draw_debug_window();
    void draw_cpu_state();
    void draw_ppu_state();
    void draw_apu_state();
    void draw_performance_stats();
    void draw_controller_state();
};
