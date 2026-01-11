#pragma once

#include <SDL.h>
#include <cstdint>
#include <vector>

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

private:
    SDL_Window* window_;
    SDL_Renderer* renderer_;
    SDL_Texture* texture_;
    bool running_;
    int width_;
    int height_;
};
