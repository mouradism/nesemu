#include "imgui_manager.hpp"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "cpu.hpp"
#include "ppu.hpp"
#include "apu.hpp"
#include "audio.hpp"
#include "controller.hpp"
#include <cmath>

ImGuiManager::ImGuiManager() = default;

ImGuiManager::~ImGuiManager() {
    if (initialized_) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }
}

bool ImGuiManager::init(SDL_Window* window, SDL_GLContext gl_context) {
    window_ = window;
    gl_context_ = gl_context;

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // Note: Docking and Viewports require imgui docking branch, disabled for now
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Setup ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    const char* glsl_version = "#version 130";
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    initialized_ = true;
    return true;
}

void ImGuiManager::new_frame() {
    if (!initialized_) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::render() {
    if (!initialized_) return;

    // Draw main debug window
    if (show_debug_window_) {
        draw_debug_window();
    }

    // Rendering
    ImGui::Render();
    // Note: OpenGL calls are handled by ImGui_ImplOpenGL3_RenderDrawData in video.cpp
}

void ImGuiManager::process_event(const SDL_Event& event) {
    if (!initialized_) return;
    ImGui_ImplSDL2_ProcessEvent(&event);
}

void ImGuiManager::set_emulator_refs(CPU* cpu, PPU* ppu, APU* apu, Audio* audio,
                                      Controller* controller1, Controller* controller2) {
    cpu_ = cpu;
    ppu_ = ppu;
    apu_ = apu;
    audio_ = audio;
    controller1_ = controller1;
    controller2_ = controller2;
}

bool ImGuiManager::wants_mouse() const {
    if (!initialized_) return false;
    return ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiManager::wants_keyboard() const {
    if (!initialized_) return false;
    return ImGui::GetIO().WantCaptureKeyboard;
}

void ImGuiManager::draw_debug_window() {
    ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("NES Emulator Debug", &show_debug_window_)) {
        ImGui::Text("NES Emulator - Debug Console");
        ImGui::Separator();

        if (ImGui::BeginTabBar("##DebugTabs")) {
            if (ImGui::BeginTabItem("Performance")) {
                draw_performance_stats();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("CPU")) {
                draw_cpu_state();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("PPU")) {
                draw_ppu_state();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("APU")) {
                draw_apu_state();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Controllers")) {
                draw_controller_state();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void ImGuiManager::draw_performance_stats() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("FPS: %.1f (%.2f ms/frame)", io.Framerate, 1000.0f / io.Framerate);
    ImGui::Text("Frame Time: %.3f ms", frame_time_ms_);
}

void ImGuiManager::draw_cpu_state() {
    if (!cpu_) {
        ImGui::Text("CPU not initialized");
        return;
    }

    ImGui::Text("=== CPU State ===");
    ImGui::Text("A:  0x%02X", cpu_->get_a());
    ImGui::Text("X:  0x%02X", cpu_->get_x());
    ImGui::Text("Y:  0x%02X", cpu_->get_y());
    ImGui::Text("SP: 0x%02X", cpu_->get_sp());
    ImGui::Text("PC: 0x%04X", cpu_->get_pc());
    ImGui::Text("Status: 0x%02X", cpu_->get_status());
    ImGui::Text("Cycles: %llu", cpu_->get_cycles());

    // Flag display
    ImGui::Separator();
    ImGui::Text("Flags:");
    ImGui::Text("  C: %d  Z: %d  I: %d  D: %d", 
        cpu_->get_flag(Flag::C), cpu_->get_flag(Flag::Z),
        cpu_->get_flag(Flag::I), cpu_->get_flag(Flag::D));
    ImGui::Text("  V: %d  N: %d",
        cpu_->get_flag(Flag::V), cpu_->get_flag(Flag::N));
}

void ImGuiManager::draw_ppu_state() {
    if (!ppu_) {
        ImGui::Text("PPU not initialized");
        return;
    }

    ImGui::Text("=== PPU State ===");
    ImGui::Text("Frame Ready: %s", ppu_->frame_ready() ? "YES" : "NO");
    ImGui::Text("NMI Occurred: %s", ppu_->nmi_occurred() ? "YES" : "NO");
    ImGui::Separator();
    ImGui::Text("Framebuffer size: %zu pixels", ppu_->get_framebuffer().size());
}

void ImGuiManager::draw_apu_state() {
    if (!apu_) {
        ImGui::Text("APU not initialized");
        return;
    }

    ImGui::Text("=== APU State ===");
    ImGui::Text("Audio output active");
    ImGui::Separator();
    ImGui::Text("Channel state and registers");
}

void ImGuiManager::draw_controller_state() {
    ImGui::Text("=== Controller State ===");

    if (controller1_) {
        ImGui::Text("Player 1:");
        ImGui::Text("  State: 0x%02X", controller1_->get_state());
        ImGui::Text("  A: %s  B: %s", 
            (controller1_->get_state() & 0x01) ? "DOWN" : "UP",
            (controller1_->get_state() & 0x02) ? "DOWN" : "UP");
    }

    if (controller2_) {
        ImGui::Text("Player 2:");
        ImGui::Text("  State: 0x%02X", controller2_->get_state());
    }
}
