#include "apu.hpp"
#include <algorithm>
#include <cmath>

// Noise period table (in CPU cycles)
const std::uint16_t APU::NOISE_PERIOD_TABLE[16] = {
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};

// Pulse waveforms (duty cycles)
const std::uint8_t APU::PULSE_WAVEFORMS[4][8] = {
    {0, 0, 0, 0, 0, 0, 0, 1},  // 12.5%
    {0, 0, 0, 0, 0, 0, 1, 1},  // 25%
    {0, 0, 0, 0, 1, 1, 1, 1},  // 50%
    {1, 1, 1, 1, 1, 1, 0, 0},  // 75%
};

// Length counter table
const std::uint8_t APU::LENGTH_COUNTER_TABLE[32] = {
    10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

// DMC rate table (in CPU cycles)
const std::uint16_t APU::DMC_RATE_TABLE[16] = {
    428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54
};

APU::APU() : sample_buffer_() {
    reset();
    sample_buffer_.reserve(8192);
}

void APU::reset() {
    pulse1_ = PulseChannel();
    pulse2_ = PulseChannel();
    triangle_ = TriangleChannel();
    noise_ = NoiseChannel();
    noise_.shift_register = 1;  // Initial LFSR value
    dmc_ = DMCChannel();
    
    enable_pulse1_ = false;
    enable_pulse2_ = false;
    enable_triangle_ = false;
    enable_noise_ = false;
    enable_dmc_ = false;
    
    frame_counter_mode_ = 0;
    frame_interrupt_inhibit_ = false;
    frame_interrupt_flag_ = false;
    frame_counter_clock_ = 0;
    
    cpu_cycles_ = 0;
    sample_index_ = 0;
    sample_buffer_.clear();
}

void APU::set_irq_callback(IRQCallback callback) {
    irq_callback_ = callback;
}

void APU::set_memory_read_callback(MemoryReadCallback callback) {
    memory_read_callback_ = callback;
}

std::uint8_t APU::read(std::uint16_t addr) {
    if (addr == 0x4015) {
        // Status register
        std::uint8_t status = 0;
        if (pulse1_.length_counter > 0) status |= 0x01;
        if (pulse2_.length_counter > 0) status |= 0x02;
        if (triangle_.length_counter > 0) status |= 0x04;
        if (noise_.length_counter > 0) status |= 0x08;
        if (dmc_.bytes_remaining > 0) status |= 0x10;
        if (frame_interrupt_flag_) status |= 0x40;
        if (dmc_.interrupt_flag) status |= 0x80;
        
        // Reading $4015 clears frame interrupt flag
        frame_interrupt_flag_ = false;
        
        return status;
    }
    return 0;
}

void APU::write(std::uint16_t addr, std::uint8_t value) {
    // Pulse 1 ($4000-$4003)
    if (addr == 0x4000) {
        pulse1_.duty = (value >> 6) & 0x03;
        pulse1_.length_counter_halt = (value & 0x20) != 0;
        pulse1_.constant_volume = (value & 0x10) != 0;
        pulse1_.volume = value & 0x0F;
    } else if (addr == 0x4001) {
        pulse1_.sweep_enabled = (value & 0x80) != 0;
        pulse1_.sweep_period = (value >> 4) & 0x07;
        pulse1_.sweep_negate = (value & 0x08) != 0;
        pulse1_.sweep_shift = value & 0x07;
        pulse1_.sweep_reload = true;
    } else if (addr == 0x4002) {
        pulse1_.period = (pulse1_.period & 0xFF00) | value;
        update_sweep_target(pulse1_, true);
    } else if (addr == 0x4003) {
        pulse1_.period = (pulse1_.period & 0x00FF) | ((value & 0x07) << 8);
        if (enable_pulse1_) {
            pulse1_.length_counter = LENGTH_COUNTER_TABLE[(value >> 3) & 0x1F];
        }
        pulse1_.start_flag = true;
        pulse1_.phase = 0;  // Reset sequencer
        update_sweep_target(pulse1_, true);
    }
    
    // Pulse 2 ($4004-$4007)
    else if (addr == 0x4004) {
        pulse2_.duty = (value >> 6) & 0x03;
        pulse2_.length_counter_halt = (value & 0x20) != 0;
        pulse2_.constant_volume = (value & 0x10) != 0;
        pulse2_.volume = value & 0x0F;
    } else if (addr == 0x4005) {
        pulse2_.sweep_enabled = (value & 0x80) != 0;
        pulse2_.sweep_period = (value >> 4) & 0x07;
        pulse2_.sweep_negate = (value & 0x08) != 0;
        pulse2_.sweep_shift = value & 0x07;
        pulse2_.sweep_reload = true;
    } else if (addr == 0x4006) {
        pulse2_.period = (pulse2_.period & 0xFF00) | value;
        update_sweep_target(pulse2_, false);
    } else if (addr == 0x4007) {
        pulse2_.period = (pulse2_.period & 0x00FF) | ((value & 0x07) << 8);
        if (enable_pulse2_) {
            pulse2_.length_counter = LENGTH_COUNTER_TABLE[(value >> 3) & 0x1F];
        }
        pulse2_.start_flag = true;
        pulse2_.phase = 0;
        update_sweep_target(pulse2_, false);
    }
    
    // Triangle ($4008-$400B)
    else if (addr == 0x4008) {
        triangle_.linear_counter_control = (value & 0x80) != 0;
        triangle_.linear_counter_reload = value & 0x7F;
    } else if (addr == 0x400A) {
        triangle_.period = (triangle_.period & 0xFF00) | value;
    } else if (addr == 0x400B) {
        triangle_.period = (triangle_.period & 0x00FF) | ((value & 0x07) << 8);
        if (enable_triangle_) {
            triangle_.length_counter = LENGTH_COUNTER_TABLE[(value >> 3) & 0x1F];
        }
        triangle_.start_flag = true;
    }
    
    // Noise ($400C-$400F)
    else if (addr == 0x400C) {
        noise_.length_counter_halt = (value & 0x20) != 0;
        noise_.constant_volume = (value & 0x10) != 0;
        noise_.volume = value & 0x0F;
    } else if (addr == 0x400E) {
        noise_.loop_flag = (value & 0x80) != 0;
        noise_.period_index = value & 0x0F;
    } else if (addr == 0x400F) {
        if (enable_noise_) {
            noise_.length_counter = LENGTH_COUNTER_TABLE[(value >> 3) & 0x1F];
        }
        noise_.start_flag = true;
    }
    
    // DMC ($4010-$4013)
    else if (addr == 0x4010) {
        dmc_.irq_enabled = (value & 0x80) != 0;
        dmc_.loop_flag = (value & 0x40) != 0;
        dmc_.rate_index = value & 0x0F;
        dmc_.timer_period = DMC_RATE_TABLE[dmc_.rate_index];
        if (!dmc_.irq_enabled) {
            dmc_.interrupt_flag = false;
        }
    } else if (addr == 0x4011) {
        dmc_.output_level = value & 0x7F;
    } else if (addr == 0x4012) {
        dmc_.sample_address = 0xC000 | (static_cast<std::uint16_t>(value) << 6);
    } else if (addr == 0x4013) {
        dmc_.sample_length = (static_cast<std::uint16_t>(value) << 4) | 1;
    }
    
    // Control register ($4015)
    else if (addr == 0x4015) {
        enable_pulse1_ = (value & 0x01) != 0;
        enable_pulse2_ = (value & 0x02) != 0;
        enable_triangle_ = (value & 0x04) != 0;
        enable_noise_ = (value & 0x08) != 0;
        enable_dmc_ = (value & 0x10) != 0;
        
        if (!enable_pulse1_) pulse1_.length_counter = 0;
        if (!enable_pulse2_) pulse2_.length_counter = 0;
        if (!enable_triangle_) triangle_.length_counter = 0;
        if (!enable_noise_) noise_.length_counter = 0;
        
        // DMC
        dmc_.interrupt_flag = false;
        if (!enable_dmc_) {
            dmc_.bytes_remaining = 0;
        } else if (dmc_.bytes_remaining == 0) {
            dmc_restart();
        }
    }
    
    // Frame counter ($4017)
    else if (addr == 0x4017) {
        frame_counter_mode_ = (value >> 7) & 0x01;
        frame_interrupt_inhibit_ = (value & 0x40) != 0;
        
        if (frame_interrupt_inhibit_) {
            frame_interrupt_flag_ = false;
        }
        
        // Reset frame counter
        frame_counter_clock_ = 0;
        
        // If mode 1, clock immediately
        if (frame_counter_mode_ == 1) {
            clock_quarter_frame();
            clock_half_frame();
        }
    }
}

void APU::update_sweep_target(PulseChannel& ch, bool is_pulse1) {
    std::int32_t delta = ch.period >> ch.sweep_shift;
    
    if (ch.sweep_negate) {
        delta = -delta;
        if (is_pulse1) {
            delta--;  // Pulse 1 uses one's complement
        }
    }
    
    std::int32_t target = static_cast<std::int32_t>(ch.period) + delta;
    ch.sweep_target = (target < 0) ? 0 : static_cast<std::uint16_t>(target);
    
    // Muting occurs if period < 8 or target > 0x7FF
    ch.sweep_muting = (ch.period < 8) || (ch.sweep_target > 0x7FF);
}

void APU::clock_sweep(PulseChannel& ch, bool is_pulse1) {
    update_sweep_target(ch, is_pulse1);
    
    if (ch.sweep_divider == 0 && ch.sweep_enabled && !ch.sweep_muting && ch.sweep_shift > 0) {
        ch.period = ch.sweep_target;
        update_sweep_target(ch, is_pulse1);
    }
    
    if (ch.sweep_divider == 0 || ch.sweep_reload) {
        ch.sweep_divider = ch.sweep_period;
        ch.sweep_reload = false;
    } else {
        ch.sweep_divider--;
    }
}

void APU::clock() {
    cpu_cycles_++;
    
    // Pulse channels: clock every 2 CPU cycles (APU runs at half CPU speed)
    if ((cpu_cycles_ & 1) == 0) {
        clock_pulse(pulse1_, true);
        clock_pulse(pulse2_, false);
        clock_noise();
    }
    
    // Triangle: clock every CPU cycle
    clock_triangle();
    
    // DMC: clock every CPU cycle
    if (enable_dmc_) {
        clock_dmc();
    }
    
    // Frame counter
    if (frame_counter_mode_ == 0) {  // 4-step mode
        if (frame_counter_clock_ == 3728 || frame_counter_clock_ == 7456 || 
            frame_counter_clock_ == 11185 || frame_counter_clock_ == 14914) {
            clock_quarter_frame();
        }
        if (frame_counter_clock_ == 7456 || frame_counter_clock_ == 14914) {
            clock_half_frame();
        }
        // Frame interrupt on step 4
        if (frame_counter_clock_ == 14914 && !frame_interrupt_inhibit_) {
            frame_interrupt_flag_ = true;
            if (irq_callback_) {
                irq_callback_();
            }
        }
        if (frame_counter_clock_ >= 14914) {
            frame_counter_clock_ = 0;
        } else {
            frame_counter_clock_++;
        }
    } else {  // 5-step mode (no interrupt)
        if (frame_counter_clock_ == 3728 || frame_counter_clock_ == 7456 || 
            frame_counter_clock_ == 11185 || frame_counter_clock_ == 18640) {
            clock_quarter_frame();
        }
        if (frame_counter_clock_ == 7456 || frame_counter_clock_ == 18640) {
            clock_half_frame();
        }
        if (frame_counter_clock_ >= 18640) {
            frame_counter_clock_ = 0;
        } else {
            frame_counter_clock_++;
        }
    }
    
    // Generate samples
    sample_index_++;
    if (sample_index_ >= (CPU_CLOCK / SAMPLE_RATE)) {
        mix_audio();
        sample_index_ = 0;
    }
}

void APU::clock_quarter_frame() {
    // Clock envelopes for pulse channels
    for (PulseChannel* ch : {&pulse1_, &pulse2_}) {
        if (ch->start_flag) {
            ch->envelope = 15;
            ch->envelope_divider = ch->volume;
            ch->start_flag = false;
        } else {
            if (ch->envelope_divider == 0) {
                ch->envelope_divider = ch->volume;
                if (ch->envelope == 0) {
                    if (ch->length_counter_halt) {
                        ch->envelope = 15;  // Loop mode
                    }
                } else {
                    ch->envelope--;
                }
            } else {
                ch->envelope_divider--;
            }
        }
    }
    
    // Noise envelope
    if (noise_.start_flag) {
        noise_.envelope = 15;
        noise_.envelope_divider = noise_.volume;
        noise_.start_flag = false;
    } else {
        if (noise_.envelope_divider == 0) {
            noise_.envelope_divider = noise_.volume;
            if (noise_.envelope == 0) {
                if (noise_.length_counter_halt) {
                    noise_.envelope = 15;
                }
            } else {
                noise_.envelope--;
            }
        } else {
            noise_.envelope_divider--;
        }
    }
    
    // Triangle linear counter
    if (triangle_.start_flag) {
        triangle_.linear_counter = triangle_.linear_counter_reload;
    } else if (triangle_.linear_counter > 0) {
        triangle_.linear_counter--;
    }
    
    if (!triangle_.linear_counter_control) {
        triangle_.start_flag = false;
    }
}

void APU::clock_half_frame() {
    // Clock length counters
    if (!pulse1_.length_counter_halt && pulse1_.length_counter > 0) {
        pulse1_.length_counter--;
    }
    if (!pulse2_.length_counter_halt && pulse2_.length_counter > 0) {
        pulse2_.length_counter--;
    }
    if (!triangle_.linear_counter_control && triangle_.length_counter > 0) {
        triangle_.length_counter--;
    }
    if (!noise_.length_counter_halt && noise_.length_counter > 0) {
        noise_.length_counter--;
    }
    
    // Clock sweep units
    clock_sweep(pulse1_, true);
    clock_sweep(pulse2_, false);
}

void APU::clock_pulse(PulseChannel& ch, bool is_pulse1) {
    if (ch.timer == 0) {
        ch.timer = ch.period;
        ch.phase = (ch.phase + 1) & 0x07;
    } else {
        ch.timer--;
    }
    
    update_pulse_output(ch);
}

void APU::clock_triangle() {
    if (triangle_.timer == 0) {
        triangle_.timer = triangle_.period;
        if (triangle_.length_counter > 0 && triangle_.linear_counter > 0) {
            triangle_.phase = (triangle_.phase + 1) & 0x1F;
        }
    } else {
        triangle_.timer--;
    }
    
    update_triangle_output();
}

void APU::clock_noise() {
    if (noise_.timer == 0) {
        noise_.timer = NOISE_PERIOD_TABLE[noise_.period_index];
        
        // Clock LFSR
        std::uint16_t feedback_bit = noise_.loop_flag ? 6 : 1;
        std::uint16_t feedback = ((noise_.shift_register >> 0) ^ (noise_.shift_register >> feedback_bit)) & 1;
        noise_.shift_register = (noise_.shift_register >> 1) | (feedback << 14);
    } else {
        noise_.timer--;
    }
    
    update_noise_output();
}

void APU::clock_dmc() {
    if (dmc_.timer == 0) {
        dmc_.timer = dmc_.timer_period;
        
        if (!dmc_.silence_flag) {
            // Output unit
            if (dmc_.shift_register & 1) {
                if (dmc_.output_level <= 125) {
                    dmc_.output_level += 2;
                }
            } else {
                if (dmc_.output_level >= 2) {
                    dmc_.output_level -= 2;
                }
            }
            dmc_.shift_register >>= 1;
        }
        
        dmc_.bits_remaining--;
        if (dmc_.bits_remaining == 0) {
            dmc_.bits_remaining = 8;
            if (dmc_.sample_buffer_empty) {
                dmc_.silence_flag = true;
            } else {
                dmc_.silence_flag = false;
                dmc_.shift_register = dmc_.sample_buffer;
                dmc_.sample_buffer_empty = true;
                dmc_read_sample();
            }
        }
    } else {
        dmc_.timer--;
    }
}

void APU::dmc_restart() {
    dmc_.current_address = dmc_.sample_address;
    dmc_.bytes_remaining = dmc_.sample_length;
}

void APU::dmc_read_sample() {
    if (dmc_.bytes_remaining > 0 && dmc_.sample_buffer_empty) {
        // Read sample from memory
        if (memory_read_callback_) {
            dmc_.sample_buffer = memory_read_callback_(dmc_.current_address);
            dmc_.sample_buffer_empty = false;
            
            // Increment address with wrap
            dmc_.current_address++;
            if (dmc_.current_address == 0) {
                dmc_.current_address = 0x8000;
            }
            
            dmc_.bytes_remaining--;
            
            if (dmc_.bytes_remaining == 0) {
                if (dmc_.loop_flag) {
                    dmc_restart();
                } else if (dmc_.irq_enabled) {
                    dmc_.interrupt_flag = true;
                    if (irq_callback_) {
                        irq_callback_();
                    }
                }
            }
        }
    }
}

void APU::update_pulse_output(PulseChannel& ch) {
    if (ch.length_counter == 0 || ch.sweep_muting) {
        ch.output = 0;
        return;
    }
    
    std::uint8_t waveform = PULSE_WAVEFORMS[ch.duty][ch.phase];
    std::uint8_t level = ch.constant_volume ? ch.volume : ch.envelope;
    ch.output = waveform ? level : 0;
}

void APU::update_triangle_output() {
    if (triangle_.length_counter == 0 || triangle_.linear_counter == 0) {
        // Keep last output to avoid popping
        return;
    }
    
    // Triangle waveform: 15,14,13,...,1,0,0,1,2,...,14,15
    static const std::uint8_t TRIANGLE_WAVEFORM[32] = {
        15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
    };
    
    triangle_.output = TRIANGLE_WAVEFORM[triangle_.phase];
}

void APU::update_noise_output() {
    if (noise_.length_counter == 0) {
        noise_.output = 0;
        return;
    }
    
    // Output is 0 if bit 0 of shift register is set
    if (noise_.shift_register & 1) {
        noise_.output = 0;
    } else {
        noise_.output = noise_.constant_volume ? noise_.volume : noise_.envelope;
    }
}

void APU::mix_audio() {
    // NES-accurate nonlinear mixing using precomputed lookup approach
    // Optimized to avoid repeated divisions
    
    // Pulse output (0-15 each, combined 0-30)
    float pulse_out = 0.0f;
    std::uint8_t pulse_sum = pulse1_.output + pulse2_.output;
    if (pulse_sum > 0) {
        // Precompute: 95.88 / (8128/x + 100) = 95.88 * x / (8128 + 100*x)
        pulse_out = (95.88f * pulse_sum) / (8128.0f + 100.0f * pulse_sum);
    }
    
    // TND output (triangle, noise, DMC) - optimized calculation
    float tnd_out = 0.0f;
    
    // Precompute divisors (these are constants: 8227, 12241, 22638)
    constexpr float TRIANGLE_SCALE = 1.0f / 8227.0f;
    constexpr float NOISE_SCALE = 1.0f / 12241.0f;
    constexpr float DMC_SCALE = 1.0f / 22638.0f;
    
    float tnd_sum = static_cast<float>(triangle_.output) * TRIANGLE_SCALE +
                    static_cast<float>(noise_.output) * NOISE_SCALE +
                    static_cast<float>(dmc_.output_level) * DMC_SCALE;
    
    if (tnd_sum > 0.0f) {
        // 159.79 / (1/x + 100) = 159.79 * x / (1 + 100*x)
        tnd_out = (159.79f * tnd_sum) / (1.0f + 100.0f * tnd_sum);
    }
    
    // Combined output - convert to -1.0 to 1.0 range
    float mixed = (pulse_out + tnd_out) * 2.0f - 1.0f;
    
    // Clamp (branchless using std::clamp)
    sample_buffer_.push_back(std::clamp(mixed, -1.0f, 1.0f));
}

std::vector<float> APU::get_samples() {
    std::vector<float> result = std::move(sample_buffer_);
    sample_buffer_.clear();
    sample_buffer_.reserve(8192);
    return result;
}
