#pragma once

#include <SDL.h>
#include <cstdint>
#include <vector>

class Audio {
public:
    Audio();
    ~Audio();

    bool init(int sample_rate = 24000, int channels = 1);
    void queue_samples(const std::vector<float>& samples);
    void update();

    bool is_initialized() const { return device_id_ != 0; }

private:
    SDL_AudioDeviceID device_id_;
    int sample_rate_;
};
