#pragma once

#include <cstdint>
#include <vector>
#include <functional>

class APU {
public:
    static constexpr int CPU_CLOCK = 1789773;  // NES CPU clock (Hz)
    static constexpr int SAMPLE_RATE = 44100;   // Audio sample rate (Hz)
    
    // Lookup tables
    static const std::uint16_t NOISE_PERIOD_TABLE[16];
    static const std::uint8_t PULSE_WAVEFORMS[4][8];
    static const std::uint8_t LENGTH_COUNTER_TABLE[32];
    static const std::uint16_t DMC_RATE_TABLE[16];
    
    // Channel structures
    struct PulseChannel {
        std::uint16_t period = 0;
        std::uint8_t duty = 0;
        std::uint8_t volume = 0;
        std::uint8_t phase = 0;
        std::uint16_t timer = 0;
        std::uint8_t length_counter = 0;
        bool length_counter_halt = false;
        bool constant_volume = false;
        std::uint8_t envelope = 0;
        std::uint8_t envelope_divider = 0;
        bool start_flag = false;
        std::uint8_t output = 0;
        // Sweep unit
        bool sweep_enabled = false;
        std::uint8_t sweep_period = 0;
        bool sweep_negate = false;
        std::uint8_t sweep_shift = 0;
        std::uint8_t sweep_divider = 0;
        bool sweep_reload = false;
        std::uint16_t sweep_target = 0;
        bool sweep_muting = false;
    };
    
    struct TriangleChannel {
        std::uint16_t period = 0;
        std::uint8_t phase = 0;
        std::uint16_t timer = 0;
        std::uint8_t length_counter = 0;
        std::uint8_t linear_counter = 0;
        std::uint8_t linear_counter_reload = 0;
        bool linear_counter_control = false;
        bool start_flag = false;
        std::uint8_t output = 0;
    };
    
    struct NoiseChannel {
        std::uint16_t shift_register = 1;
        std::uint8_t period_index = 0;
        std::uint8_t volume = 0;
        std::uint8_t length_counter = 0;
        bool length_counter_halt = false;
        bool constant_volume = false;
        std::uint8_t envelope = 0;
        std::uint8_t envelope_divider = 0;
        bool start_flag = false;
        std::uint8_t output = 0;
        bool loop_flag = false;
        std::uint16_t timer = 0;
    };
    
    struct DMCChannel {
        bool irq_enabled = false;
        bool loop_flag = false;
        std::uint8_t rate_index = 0;
        std::uint16_t timer = 0;
        std::uint16_t timer_period = 0;
        
        // Memory reader
        std::uint16_t sample_address = 0xC000;
        std::uint16_t sample_length = 0;
        std::uint16_t current_address = 0;
        std::uint16_t bytes_remaining = 0;
        
        // Output unit
        std::uint8_t sample_buffer = 0;
        bool sample_buffer_empty = true;
        std::uint8_t shift_register = 0;
        std::uint8_t bits_remaining = 0;
        std::uint8_t output_level = 0;
        bool silence_flag = true;
        
        bool interrupt_flag = false;
    };
    
    // IRQ callback type
    using IRQCallback = std::function<void()>;
    
    APU();
    
    void reset();
    std::uint8_t read(std::uint16_t addr);
    void write(std::uint16_t addr, std::uint8_t value);
    void clock();
    std::vector<float> get_samples();
    
    // Set IRQ callback for frame counter and DMC interrupts
    void set_irq_callback(IRQCallback callback);
    
    // Memory read callback for DMC (needs access to CPU memory)
    using MemoryReadCallback = std::function<std::uint8_t(std::uint16_t)>;
    void set_memory_read_callback(MemoryReadCallback callback);
    
    // Check interrupt status
    bool get_frame_interrupt() const { return frame_interrupt_flag_; }
    bool get_dmc_interrupt() const { return dmc_.interrupt_flag; }

private:
    PulseChannel pulse1_;
    PulseChannel pulse2_;
    TriangleChannel triangle_;
    NoiseChannel noise_;
    DMCChannel dmc_;
    
    bool enable_pulse1_;
    bool enable_pulse2_;
    bool enable_triangle_;
    bool enable_noise_;
    bool enable_dmc_;
    
    std::uint8_t frame_counter_mode_;
    bool frame_interrupt_inhibit_;
    bool frame_interrupt_flag_;
    std::uint32_t frame_counter_clock_;
    
    std::uint32_t cpu_cycles_;
    std::uint32_t sample_index_;
    std::vector<float> sample_buffer_;
    
    IRQCallback irq_callback_;
    MemoryReadCallback memory_read_callback_;
    
    void clock_quarter_frame();
    void clock_half_frame();
    void clock_pulse(PulseChannel& ch, bool is_pulse1);
    void clock_triangle();
    void clock_noise();
    void clock_dmc();
    void update_pulse_output(PulseChannel& ch);
    void update_triangle_output();
    void update_noise_output();
    void mix_audio();
    
    // Sweep unit helpers
    void update_sweep_target(PulseChannel& ch, bool is_pulse1);
    void clock_sweep(PulseChannel& ch, bool is_pulse1);
    
    // DMC helpers
    void dmc_restart();
    void dmc_read_sample();
};
