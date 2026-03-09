#include "imgui_manager.hpp"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "ImGuiFileDialog.h"
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

    // Handle file dialog
    if (show_file_dialog_) {
        ImGuiFileDialog::Instance()->OpenDialog("ChooseROM", "Choose ROM File", ".nes", ".");
    }

    if (ImGuiFileDialog::Instance()->Display("ChooseROM")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            selected_rom_path_ = ImGuiFileDialog::Instance()->GetFilePathName();
            rom_load_requested_ = true;
        }
        ImGuiFileDialog::Instance()->Close();
        show_file_dialog_ = false;
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

        if (ImGui::Button("Load ROM")) {
            show_file_dialog_ = true;
        }

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
    
    // Current instruction being executed
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Current Instruction:");
    ImGui::Text("  Opcode:   0x%02X", cpu_->get_opcode());
    ImGui::Text("  Mnemonic: %s", cpu_->get_instruction_name());
    ImGui::Text("  Address:  0x%04X", cpu_->get_addr_abs());
    ImGui::Text("  Fetched:  0x%02X", cpu_->get_fetched());
    
    ImGui::Separator();
    
    // Registers display
    ImGui::TextColored(ImVec4(0, 1, 1, 1), "Registers:");
    ImGui::Text("  Accumulator (A):  0x%02X (%3d)", cpu_->get_a(), cpu_->get_a());
    ImGui::Text("  X Register:       0x%02X (%3d)", cpu_->get_x(), cpu_->get_x());
    ImGui::Text("  Y Register:       0x%02X (%3d)", cpu_->get_y(), cpu_->get_y());
    ImGui::Text("  Stack Pointer:    0x%02X (0x01%02X)", cpu_->get_sp(), cpu_->get_sp());
    ImGui::Text("  Program Counter:  0x%04X", cpu_->get_pc());
    ImGui::Text("  Total Cycles:     %llu", cpu_->get_cycles());
    
    ImGui::Separator();
    
    // CPU Usage metrics
    ImGui::TextColored(ImVec4(0, 1, 1, 1), "CPU Performance:");
    
    // Calculate cycles per second (approx) - persistent state
    static std::uint64_t last_cycles = 0;
    static float last_time = 0.0f;
    static float cached_mhz = 0.0f;
    static float cached_instr_per_sec = 0.0f;
    
    float current_time = ImGui::GetTime();
    
    if (current_time - last_time >= 1.0f) {
        std::uint64_t current_cycles = cpu_->get_cycles();
        float cycles_per_second = static_cast<float>(current_cycles - last_cycles);
        cached_mhz = cycles_per_second / 1000000.0f;
        cached_instr_per_sec = cycles_per_second / 3.0f;  // ~3 cycles per instruction average
        
        last_cycles = current_cycles;
        last_time = current_time;
    }
    
    ImGui::Text("  Cycles/sec:       %.2f MHz", cached_mhz);
    ImGui::Text("  Instructions/sec: ~%.0fk", cached_instr_per_sec / 1000.0f);
    
    ImGui::Separator();
    
    // Flags display
    ImGui::TextColored(ImVec4(0, 1, 1, 1), "Status Flags: 0x%02X", cpu_->get_status());
    
    // Each flag on its own line with colored indicator
    ImGui::TextColored(
        cpu_->get_flag(Flag::C) ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1),
        "  C: %d  Carry", cpu_->get_flag(Flag::C)
    );
    ImGui::TextColored(
        cpu_->get_flag(Flag::Z) ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1),
        "  Z: %d  Zero", cpu_->get_flag(Flag::Z)
    );
    ImGui::TextColored(
        cpu_->get_flag(Flag::I) ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1),
        "  I: %d  Interrupt Disable", cpu_->get_flag(Flag::I)
    );
    ImGui::TextColored(
        cpu_->get_flag(Flag::D) ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1),
        "  D: %d  Decimal", cpu_->get_flag(Flag::D)
    );
    ImGui::TextColored(
        cpu_->get_flag(Flag::B) ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1),
        "  B: %d  Break", cpu_->get_flag(Flag::B)
    );
    ImGui::TextColored(
        cpu_->get_flag(Flag::U) ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1),
        "  U: %d  Unused (always 1)", cpu_->get_flag(Flag::U)
    );
    ImGui::TextColored(
        cpu_->get_flag(Flag::V) ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1),
        "  V: %d  Overflow", cpu_->get_flag(Flag::V)
    );
    ImGui::TextColored(
        cpu_->get_flag(Flag::N) ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1),
        "  N: %d  Negative", cpu_->get_flag(Flag::N)
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
