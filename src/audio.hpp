#pragma once

#include <SDL.h>
#include <cstdint>
#include <vector>

class Audio {
public:
    Audio();
    ~Audio();

    bool init(int sample_rate = 44100, int channels = 1);  // Match APU sample rate
    void queue_samples(const std::vector<float>& samples);
    void update();
    
    // New methods
    void pause();
    void resume();
    void set_volume(float volume);  // 0.0 to 1.0
    std::uint32_t get_queued_size() const;
    bool is_buffer_low() const;  // Check if we need more samples

    bool is_initialized() const { return device_id_ != 0; }
    int get_sample_rate() const { return sample_rate_; }

private:
    SDL_AudioDeviceID device_id_;
    int sample_rate_;
    float volume_;
    bool paused_;
    
    static constexpr std::uint32_t MIN_BUFFER_SIZE = 2048;  // Minimum samples to avoid underrun
    static constexpr std::uint32_t MAX_BUFFER_SIZE = 8192;  // Maximum samples to avoid latency
};
