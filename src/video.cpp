#include "video.hpp"
#include "imgui_manager.hpp"
#include "imgui.h"
#include "imgui_impl_opengl3.h"

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

#include <iostream>

Video::Video()
    : window_(nullptr),
      gl_context_(nullptr),
      game_texture_(0),
      running_(false),
      width_(256),
      height_(240),
      scale_(2) {}

Video::~Video() {
    if (game_texture_ != 0) {
        glDeleteTextures(1, &game_texture_);
    }
    if (gl_context_) {
        SDL_GL_DeleteContext(gl_context_);
    }
    if (window_) {
        SDL_DestroyWindow(window_);
    }
}

bool Video::init(int width, int height, int scale) {
    width_ = width;
    height_ = height;
    scale_ = scale;

    // Initialize framebuffer
    framebuffer_.resize(width * height, 0xFF000000);

    std::cout << "Creating SDL2 window with OpenGL...\n";
    
    // Set OpenGL attributes before creating window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    
    window_ = SDL_CreateWindow(
        "nesemu",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width * scale + 520,  // Extra space for debug window
        height * scale + 40,
        SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    if (!window_) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
        return false;
    }
    std::cout << "Window created successfully.\n";

    // Create OpenGL context
    std::cout << "Creating OpenGL context...\n";
    gl_context_ = SDL_GL_CreateContext(window_);
    if (!gl_context_) {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << '\n';
        return false;
    }

    SDL_GL_MakeCurrent(window_, gl_context_);
    SDL_GL_SetSwapInterval(1);  // Enable VSync
    std::cout << "OpenGL context created successfully.\n";

    // Create texture for game framebuffer
    create_game_texture();

    // Initialize ImGui
    std::cout << "Initializing ImGui...\n";
    imgui_manager_ = std::make_unique<ImGuiManager>();
    if (!imgui_manager_->init(window_, gl_context_)) {
        std::cerr << "ImGui initialization failed\n";
        return false;
    }
    std::cout << "ImGui initialized successfully.\n";

    running_ = true;
    return true;
}

void Video::create_game_texture() {
    glGenTextures(1, &game_texture_);
    glBindTexture(GL_TEXTURE_2D, game_texture_);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    
    // Create empty texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Video::update_frame(const std::vector<std::uint32_t>& framebuffer) {
    if (framebuffer.size() != framebuffer_.size()) {
        return;
    }
    
    // Convert ARGB to RGBA for OpenGL
    for (size_t i = 0; i < framebuffer.size(); ++i) {
        std::uint32_t argb = framebuffer[i];
        std::uint8_t a = (argb >> 24) & 0xFF;
        std::uint8_t r = (argb >> 16) & 0xFF;
        std::uint8_t g = (argb >> 8) & 0xFF;
        std::uint8_t b = argb & 0xFF;
        framebuffer_[i] = (a << 24) | (b << 16) | (g << 8) | r;  // ABGR for GL_RGBA
    }
    
    // Update OpenGL texture
    glBindTexture(GL_TEXTURE_2D, game_texture_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer_.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Video::render_game_window() {
    // Render game window in ImGui
    ImGui::SetNextWindowPos(ImVec2(520.0f, 10.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(width_ * scale_ + 20), 
                                     static_cast<float>(height_ * scale_ + 40)), ImGuiCond_FirstUseEver);

    ImGui::Begin("Game", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    
    // Get available size in window
    ImVec2 available = ImGui::GetContentRegionAvail();
    
    // Calculate size maintaining aspect ratio
    float aspect = static_cast<float>(width_) / static_cast<float>(height_);
    float display_width = available.x;
    float display_height = display_width / aspect;
    
    if (display_height > available.y) {
        display_height = available.y;
        display_width = display_height * aspect;
    }
    
    // Center the image
    float offset_x = (available.x - display_width) * 0.5f;
    if (offset_x > 0) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset_x);
    }
    
    // Render the game texture
    ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(game_texture_)),
                 ImVec2(display_width, display_height),
                 ImVec2(0, 0), ImVec2(1, 1));  // UV coords
    
    ImGui::End();
}

void Video::render() {
    // Clear the screen
    int w, h;
    SDL_GetWindowSize(window_, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Start ImGui frame
    imgui_manager_->new_frame();

    // Render game window with the NES framebuffer
    render_game_window();

    // Render ImGui debug UI
    imgui_manager_->render();

    // Draw ImGui
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Swap buffers
    SDL_GL_SwapWindow(window_);
}

bool Video::handle_events() {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        imgui_manager_->process_event(ev);

        if (ev.type == SDL_QUIT) {
            std::cout << "Quit event received.\n";
            running_ = false;
            return false;
        } else if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE) {
            std::cout << "ESC pressed.\n";
            running_ = false;
            return false;
        } else if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_F1) {
            // F1 to toggle debug window
            imgui_manager_->toggle_debug_window();
        }
    }
    return true;
}
