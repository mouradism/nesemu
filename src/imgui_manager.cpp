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
    ImGui::Separator();
    
    // Registers display (without columns to avoid tab issues)
    ImGui::BeginGroup();
    ImGui::Text("Accumulator (A): 0x%02X (%3d)", cpu_->get_a(), cpu_->get_a());
    ImGui::Text("X Register:     0x%02X (%3d)", cpu_->get_x(), cpu_->get_x());
    ImGui::Text("Y Register:     0x%02X (%3d)", cpu_->get_y(), cpu_->get_y());
    ImGui::Text("Stack Pointer:  0x%02X (0x01%02X)", cpu_->get_sp(), cpu_->get_sp());
    ImGui::Text("Program Counter: 0x%04X", cpu_->get_pc());
    ImGui::Text("Cycles:          %llu", cpu_->get_cycles());
    ImGui::EndGroup();
    
    ImGui::Separator();
    
    // Flags display
    ImGui::Text("Status Flags: 0x%02X", cpu_->get_status());
    ImGui::Separator();
    
    // Flag breakdown with colored indicators
    ImGui::BeginGroup();
    
    // Row 1
    ImGui::TextColored(
        cpu_->get_flag(Flag::C) ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1),
        "C: %d Carry", cpu_->get_flag(Flag::C)
    );
    ImGui::SameLine(100);
    ImGui::TextColored(
        cpu_->get_flag(Flag::Z) ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1),
        "Z: %d Zero", cpu_->get_flag(Flag::Z)
    );
    ImGui::SameLine(200);
    ImGui::TextColored(
        cpu_->get_flag(Flag::I) ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1),
        "I: %d Interrupt", cpu_->get_flag(Flag::I)
    );
    ImGui::SameLine(340);
    ImGui::TextColored(
        cpu_->get_flag(Flag::D) ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1),
        "D: %d Decimal", cpu_->get_flag(Flag::D)
    );
    
    // Row 2
    ImGui::TextColored(
        cpu_->get_flag(Flag::B) ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1),
        "B: %d Break", cpu_->get_flag(Flag::B)
    );
    ImGui::SameLine(100);
    ImGui::TextColored(
        cpu_->get_flag(Flag::V) ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1),
        "V: %d Overflow", cpu_->get_flag(Flag::V)
    );
    ImGui::SameLine(200);
    ImGui::TextColored(
        cpu_->get_flag(Flag::N) ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1),
        "N: %d Negative", cpu_->get_flag(Flag::N)
    );
    ImGui::SameLine(340);
    ImGui::TextColored(
        cpu_->get_flag(Flag::U) ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1),
        "U: %d Unused", cpu_->get_flag(Flag::U)
    );
    
    ImGui::EndGroup();
    ImGui::Separator();
    
    // Flag explanations
    ImGui::TextWrapped(
        "Flags: C=Carry, Z=Zero, I=InterruptDisable, D=Decimal, "
        "B=Break, V=Overflow, N=Negative, U=Unused"
    );
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
