#pragma once

#include <cstdint>

// NES Controller buttons
enum class Button : std::uint8_t {
    A      = 0,
    B      = 1,
    Select = 2,
    Start  = 3,
    Up     = 4,
    Down   = 5,
    Left   = 6,
    Right  = 7
};

class Controller {
public:
    Controller() = default;
    
    // Set button state (pressed = true)
    void set_button(Button button, bool pressed) {
        std::uint8_t mask = 1 << static_cast<std::uint8_t>(button);
        if (pressed) {
            button_state_ |= mask;
        } else {
            button_state_ &= ~mask;
        }
    }
    
    // Set all buttons at once (bit 0 = A, bit 7 = Right)
    void set_state(std::uint8_t state) {
        button_state_ = state;
    }
    
    // Get current button state
    std::uint8_t get_state() const {
        return button_state_;
    }
    
    // Check if a specific button is pressed
    bool is_pressed(Button button) const {
        std::uint8_t mask = 1 << static_cast<std::uint8_t>(button);
        return (button_state_ & mask) != 0;
    }
    
    // Write to controller register ($4016)
    // Bit 0 controls strobe
    void write(std::uint8_t value) {
        bool new_strobe = (value & 0x01) != 0;
        
        // On strobe falling edge (1 -> 0), latch button state
        if (strobe_ && !new_strobe) {
            shift_register_ = button_state_;
        }
        
        strobe_ = new_strobe;
        
        // While strobe is high, continuously reload
        if (strobe_) {
            shift_register_ = button_state_;
        }
    }
    
    // Read from controller register ($4016 or $4017)
    // Returns bit 0 = current button, shifts to next
    std::uint8_t read() {
        std::uint8_t result = 0;
        
        if (strobe_) {
            // While strobe is high, always return A button
            result = button_state_ & 0x01;
        } else {
            // Return current bit and shift
            result = shift_register_ & 0x01;
            shift_register_ >>= 1;
            // After 8 reads, returns 1s (open bus behavior)
            shift_register_ |= 0x80;
        }
        
        // Only bit 0 is the button state, bits 1-4 are open bus (return 0x40 typically)
        return result | 0x40;
    }
    
    void reset() {
        button_state_ = 0;
        shift_register_ = 0;
        strobe_ = false;
    }

private:
    std::uint8_t button_state_ = 0;    // Current button states
    std::uint8_t shift_register_ = 0;  // Shift register for serial read
    bool strobe_ = false;              // Strobe latch state
};
