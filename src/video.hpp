#pragma once

#include <SDL.h>
#include <cstdint>
#include <vector>
#include <memory>

class ImGuiManager;

class Video {
public:
    Video();
    ~Video();

    bool init(int width = 256, int height = 240, int scale = 2);
    void update_frame(const std::vector<std::uint32_t>& framebuffer);
    void render();
    bool handle_events();
    
    bool is_running() const { return running_; }
    void quit() { running_ = false; }

    // ImGui access
    ImGuiManager* get_imgui_manager() { return imgui_manager_.get(); }

private:
    SDL_Window* window_;
    SDL_GLContext gl_context_;
    std::unique_ptr<ImGuiManager> imgui_manager_;
    
    // OpenGL texture for game framebuffer
    unsigned int game_texture_;
    std::vector<std::uint32_t> framebuffer_;
    
    bool running_;
    int width_;
    int height_;
    int scale_;

    void render_game_window();
    void create_game_texture();
};
