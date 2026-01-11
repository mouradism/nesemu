#include "video.hpp"
#include <iostream>

Video::Video()
    : window_(nullptr),
      renderer_(nullptr),
      texture_(nullptr),
      running_(false),
      width_(256),
      height_(240) {}

Video::~Video() {
    if (texture_) SDL_DestroyTexture(texture_);
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
}

bool Video::init(int width, int height, int scale) {
    width_ = width;
    height_ = height;

    std::cout << "Creating window...\n";
    window_ = SDL_CreateWindow(
        "nesemu",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width * scale,
        height * scale,
        SDL_WINDOW_SHOWN
    );
    if (!window_) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
        return false;
    }
    std::cout << "Window created successfully.\n";

    std::cout << "Creating renderer...\n";
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << '\n';
        return false;
    }
    std::cout << "Renderer created successfully.\n";

    std::cout << "Creating texture...\n";
    texture_ = SDL_CreateTexture(
        renderer_,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height
    );
    if (!texture_) {
        std::cerr << "SDL_CreateTexture failed: " << SDL_GetError() << '\n';
        return false;
    }
    std::cout << "Texture created successfully.\n";

    running_ = true;
    return true;
}

void Video::update_frame(const std::vector<std::uint32_t>& framebuffer) {
    SDL_UpdateTexture(texture_, nullptr, framebuffer.data(), width_ * static_cast<int>(sizeof(std::uint32_t)));
}

void Video::render() {
    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
}

bool Video::handle_events() {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) {
            std::cout << "Quit event received.\n";
            running_ = false;
            return false;
        } else if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE) {
            std::cout << "ESC pressed.\n";
            running_ = false;
            return false;
        }
    }
    return true;
}
