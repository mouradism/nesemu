#include "audio.hpp"
#include <iostream>

Audio::Audio()
    : device_id_(0),
      sample_rate_(48000) {}

Audio::~Audio() {
    if (device_id_ != 0) {
        SDL_CloseAudioDevice(device_id_);
    }
}

bool Audio::init(int sample_rate, int channels) {
    sample_rate_ = sample_rate;

    std::cout << "Opening audio device...\n";
    SDL_AudioSpec desired{};
    SDL_AudioSpec obtained{};
    desired.freq = sample_rate;
    desired.format = AUDIO_F32SYS;
    desired.channels = channels;
    desired.samples = 1024;

    device_id_ = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (device_id_ == 0) {
        std::cerr << "SDL_OpenAudioDevice failed: " << SDL_GetError() << '\n';
        return false;
    }

    std::cout << "Audio device opened successfully.\n";
    SDL_PauseAudioDevice(device_id_, 0);
    return true;
}

void Audio::queue_samples(const std::vector<float>& samples) {
    if (device_id_ != 0) {
        SDL_QueueAudio(device_id_, samples.data(), samples.size() * sizeof(float));
    }
}

void Audio::update() {
    // Placeholder for future audio frame management
    // Can be extended to monitor queue levels or handle underruns
}
