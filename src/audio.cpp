#include "audio.hpp"
#include <iostream>
#include <algorithm>

Audio::Audio()
    : device_id_(0),
      sample_rate_(44100),
      volume_(1.0f),
      paused_(false) {
    // Pre-allocate volume adjustment buffer
    adjusted_buffer_.reserve(2048);
}

Audio::~Audio() {
    if (device_id_ != 0) {
        SDL_CloseAudioDevice(device_id_);
    }
}

bool Audio::init(int sample_rate, int channels) {
    sample_rate_ = sample_rate;

    std::cout << "Opening audio device at " << sample_rate << " Hz...\n";
    SDL_AudioSpec desired{};
    SDL_AudioSpec obtained{};
    desired.freq = sample_rate;
    desired.format = AUDIO_F32SYS;
    desired.channels = static_cast<Uint8>(channels);
    desired.samples = 512;  // Reduced buffer size for lower latency (~12ms at 44100Hz)

    device_id_ = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (device_id_ == 0) {
        std::cerr << "SDL_OpenAudioDevice failed: " << SDL_GetError() << '\n';
        return false;
    }

    std::cout << "Audio device opened successfully.\n";
    std::cout << "  Frequency: " << obtained.freq << " Hz\n";
    std::cout << "  Channels: " << static_cast<int>(obtained.channels) << "\n";
    std::cout << "  Samples: " << obtained.samples << "\n";
    
    // Start playback
    SDL_PauseAudioDevice(device_id_, 0);
    paused_ = false;
    
    return true;
}

void Audio::queue_samples(const std::vector<float>& samples) {
    if (device_id_ == 0 || samples.empty()) {
        return;
    }
    
    // Check if buffer is getting too full - skip samples to reduce latency
    std::uint32_t queued = get_queued_size();
    if (queued > MAX_BUFFER_SIZE) {
        // Buffer is backing up, skip these samples to catch up
        return;
    }
    
    // Fast path: if volume is ~1.0, queue directly without copy
    if (volume_ >= 0.99f) {
        SDL_QueueAudio(device_id_, samples.data(), 
                       static_cast<Uint32>(samples.size() * sizeof(float)));
    } else {
        // Apply volume - reuse pre-allocated buffer to avoid allocation
        adjusted_buffer_.resize(samples.size());
        for (size_t i = 0; i < samples.size(); ++i) {
            adjusted_buffer_[i] = samples[i] * volume_;
        }
        SDL_QueueAudio(device_id_, adjusted_buffer_.data(), 
                       static_cast<Uint32>(adjusted_buffer_.size() * sizeof(float)));
    }
}

void Audio::update() {
    // Monitor buffer levels to prevent underrun/overflow
    if (device_id_ == 0) return;
    
    std::uint32_t queued = get_queued_size();
    
    // Warning if buffer is getting low
    if (queued < MIN_BUFFER_SIZE / 2) {
        // Could trigger a callback or flag here for the emulator to generate more samples
    }
    
    // If buffer is too full, we might be generating samples too fast
    // This can cause latency - the emulator might need to slow down
    if (queued > MAX_BUFFER_SIZE) {
        // Audio is backing up - could clear some or signal emulator
    }
}

void Audio::pause() {
    if (device_id_ != 0) {
        SDL_PauseAudioDevice(device_id_, 1);
        paused_ = true;
    }
}

void Audio::resume() {
    if (device_id_ != 0) {
        SDL_PauseAudioDevice(device_id_, 0);
        paused_ = false;
    }
}

void Audio::set_volume(float volume) {
    volume_ = std::clamp(volume, 0.0f, 1.0f);
}

std::uint32_t Audio::get_queued_size() const {
    if (device_id_ == 0) return 0;
    return SDL_GetQueuedAudioSize(device_id_) / sizeof(float);
}

bool Audio::is_buffer_low() const {
    return get_queued_size() < MIN_BUFFER_SIZE;
}
