#pragma once

#include "controller.hpp"
#include <SDL.h>
#include <unordered_map>

// Maps SDL keyboard input to NES controller buttons
class Input {
public:
    Input() {
        // Default keyboard mapping for Player 1
        // Arrow keys for D-pad, Z=A, X=B, Enter=Start, RShift=Select
        p1_keymap_[SDLK_z] = Button::A;
        p1_keymap_[SDLK_x] = Button::B;
        p1_keymap_[SDLK_RSHIFT] = Button::Select;
        p1_keymap_[SDLK_RETURN] = Button::Start;
        p1_keymap_[SDLK_UP] = Button::Up;
        p1_keymap_[SDLK_DOWN] = Button::Down;
        p1_keymap_[SDLK_LEFT] = Button::Left;
        p1_keymap_[SDLK_RIGHT] = Button::Right;
        
        // Alternative WASD mapping also for P1
        p1_keymap_[SDLK_a] = Button::A;
        p1_keymap_[SDLK_s] = Button::B;
        p1_keymap_[SDLK_w] = Button::Up;
        // Note: 'a' conflicts, so using different keys for WASD movement
        
        // Player 2 (numpad or IJKL)
        p2_keymap_[SDLK_KP_0] = Button::A;
        p2_keymap_[SDLK_KP_PERIOD] = Button::B;
        p2_keymap_[SDLK_KP_ENTER] = Button::Start;
        p2_keymap_[SDLK_KP_PLUS] = Button::Select;
        p2_keymap_[SDLK_KP_8] = Button::Up;
        p2_keymap_[SDLK_KP_2] = Button::Down;
        p2_keymap_[SDLK_KP_4] = Button::Left;
        p2_keymap_[SDLK_KP_6] = Button::Right;
    }
    
    // Set controller references
    void set_controllers(Controller* p1, Controller* p2) {
        controller1_ = p1;
        controller2_ = p2;
    }
    
    // Process a single SDL event for controller input
    // Returns true if event was handled as input
    bool process_event(const SDL_Event& event) {
        if (event.type != SDL_KEYDOWN && event.type != SDL_KEYUP) {
            return false;
        }
        
        bool pressed = (event.type == SDL_KEYDOWN);
        SDL_Keycode key = event.key.keysym.sym;
        
        // Check Player 1 mapping
        if (controller1_) {
            auto it = p1_keymap_.find(key);
            if (it != p1_keymap_.end()) {
                controller1_->set_button(it->second, pressed);
                return true;
            }
        }
        
        // Check Player 2 mapping
        if (controller2_) {
            auto it = p2_keymap_.find(key);
            if (it != p2_keymap_.end()) {
                controller2_->set_button(it->second, pressed);
                return true;
            }
        }
        
        return false;
    }
    
    // Update controllers from current keyboard state (alternative to event-based)
    void update_from_keyboard_state() {
        const Uint8* state = SDL_GetKeyboardState(nullptr);
        
        if (controller1_) {
            controller1_->set_button(Button::A,      state[SDL_SCANCODE_Z] != 0);
            controller1_->set_button(Button::B,      state[SDL_SCANCODE_X] != 0);
            controller1_->set_button(Button::Select, state[SDL_SCANCODE_RSHIFT] != 0);
            controller1_->set_button(Button::Start,  state[SDL_SCANCODE_RETURN] != 0);
            controller1_->set_button(Button::Up,     state[SDL_SCANCODE_UP] != 0);
            controller1_->set_button(Button::Down,   state[SDL_SCANCODE_DOWN] != 0);
            controller1_->set_button(Button::Left,   state[SDL_SCANCODE_LEFT] != 0);
            controller1_->set_button(Button::Right,  state[SDL_SCANCODE_RIGHT] != 0);
        }
        
        if (controller2_) {
            controller2_->set_button(Button::A,      state[SDL_SCANCODE_KP_0] != 0);
            controller2_->set_button(Button::B,      state[SDL_SCANCODE_KP_PERIOD] != 0);
            controller2_->set_button(Button::Select, state[SDL_SCANCODE_KP_PLUS] != 0);
            controller2_->set_button(Button::Start,  state[SDL_SCANCODE_KP_ENTER] != 0);
            controller2_->set_button(Button::Up,     state[SDL_SCANCODE_KP_8] != 0);
            controller2_->set_button(Button::Down,   state[SDL_SCANCODE_KP_2] != 0);
            controller2_->set_button(Button::Left,   state[SDL_SCANCODE_KP_4] != 0);
            controller2_->set_button(Button::Right,  state[SDL_SCANCODE_KP_6] != 0);
        }
    }
    
    // Remap a key for player 1
    void remap_p1(SDL_Keycode key, Button button) {
        p1_keymap_[key] = button;
    }
    
    // Remap a key for player 2
    void remap_p2(SDL_Keycode key, Button button) {
        p2_keymap_[key] = button;
    }
    
    // Get current key mapping description
    static const char* get_default_mapping() {
        return "Player 1: Arrows=D-Pad, Z=A, X=B, Enter=Start, RShift=Select\n"
               "Player 2: Numpad 8/2/4/6=D-Pad, 0=A, .=B, Enter=Start, +=Select\n"
               "ESC=Quit";
    }

private:
    Controller* controller1_ = nullptr;
    Controller* controller2_ = nullptr;
    
    std::unordered_map<SDL_Keycode, Button> p1_keymap_;
    std::unordered_map<SDL_Keycode, Button> p2_keymap_;
};
