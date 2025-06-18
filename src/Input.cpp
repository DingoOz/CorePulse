#include "Input.h"
#include <iostream>

namespace CorePulse {

Input::Input() {
    // Initialize mouse state
    mouse_state_ = {};
    
    // Get initial mouse position
    SDL_GetMouseState(&mouse_state_.x, &mouse_state_.y);
}

void Input::update() {
    reset_frame_data();
    update_key_states();
    update_mouse_states();
}

void Input::handle_event(const SDL_Event& event) {
    switch (event.type) {
        case SDL_KEYDOWN:
            if (!event.key.repeat) {
                keyboard_state_[event.key.keysym.scancode] = KeyState::Pressed;
                keys_pressed_this_frame_.insert(event.key.keysym.scancode);
            }
            break;
            
        case SDL_KEYUP:
            keyboard_state_[event.key.keysym.scancode] = KeyState::Released;
            keys_released_this_frame_.insert(event.key.keysym.scancode);
            break;
            
        case SDL_MOUSEBUTTONDOWN: {
            auto button = static_cast<MouseButton>(event.button.button);
            mouse_state_.buttons[event.button.button - 1] = KeyState::Pressed;
            mouse_pressed_this_frame_.insert(button);
            break;
        }
        
        case SDL_MOUSEBUTTONUP: {
            auto button = static_cast<MouseButton>(event.button.button);
            mouse_state_.buttons[event.button.button - 1] = KeyState::Released;
            mouse_released_this_frame_.insert(button);
            break;
        }
        
        case SDL_MOUSEMOTION:
            mouse_state_.delta_x = event.motion.xrel;
            mouse_state_.delta_y = event.motion.yrel;
            mouse_state_.x = event.motion.x;
            mouse_state_.y = event.motion.y;
            break;
            
        case SDL_MOUSEWHEEL:
            mouse_state_.wheel_x = event.wheel.x;
            mouse_state_.wheel_y = event.wheel.y;
            break;
    }
}

bool Input::is_key_pressed(SDL_Scancode key) const {
    return keys_pressed_this_frame_.contains(key);
}

bool Input::is_key_held(SDL_Scancode key) const {
    auto it = keyboard_state_.find(key);
    return it != keyboard_state_.end() && 
           (it->second == KeyState::Held || it->second == KeyState::Pressed);
}

bool Input::is_key_released(SDL_Scancode key) const {
    return keys_released_this_frame_.contains(key);
}

KeyState Input::get_key_state(SDL_Scancode key) const {
    auto it = keyboard_state_.find(key);
    return it != keyboard_state_.end() ? it->second : KeyState::Released;
}

bool Input::is_mouse_button_pressed(MouseButton button) const {
    return mouse_pressed_this_frame_.contains(button);
}

bool Input::is_mouse_button_held(MouseButton button) const {
    size_t index = static_cast<size_t>(button) - 1;
    if (index >= mouse_state_.buttons.size()) return false;
    
    KeyState state = mouse_state_.buttons[index];
    return state == KeyState::Held || state == KeyState::Pressed;
}

bool Input::is_mouse_button_released(MouseButton button) const {
    return mouse_released_this_frame_.contains(button);
}

KeyState Input::get_mouse_button_state(MouseButton button) const {
    size_t index = static_cast<size_t>(button) - 1;
    if (index >= mouse_state_.buttons.size()) return KeyState::Released;
    
    return mouse_state_.buttons[index];
}

void Input::set_relative_mouse_mode(bool enabled) {
    SDL_SetRelativeMouseMode(enabled ? SDL_TRUE : SDL_FALSE);
}

bool Input::is_relative_mouse_mode() const {
    return SDL_GetRelativeMouseMode() == SDL_TRUE;
}

void Input::update_key_states() {
    // Update held keys
    for (auto& [key, state] : keyboard_state_) {
        if (state == KeyState::Pressed) {
            state = KeyState::Held;
        }
    }
}

void Input::update_mouse_states() {
    // Update held mouse buttons
    for (auto& state : mouse_state_.buttons) {
        if (state == KeyState::Pressed) {
            state = KeyState::Held;
        }
    }
}

void Input::reset_frame_data() {
    keys_pressed_this_frame_.clear();
    keys_released_this_frame_.clear();
    mouse_pressed_this_frame_.clear();
    mouse_released_this_frame_.clear();
    
    // Reset mouse wheel and delta
    mouse_state_.wheel_x = 0;
    mouse_state_.wheel_y = 0;
    mouse_state_.delta_x = 0;
    mouse_state_.delta_y = 0;
    
    // Clean up released keys from the state map
    auto it = keyboard_state_.begin();
    while (it != keyboard_state_.end()) {
        if (it->second == KeyState::Released) {
            it = keyboard_state_.erase(it);
        } else {
            ++it;
        }
    }
    
    // Clean up released mouse buttons
    for (auto& state : mouse_state_.buttons) {
        if (state == KeyState::Released) {
            state = KeyState::Released;
        }
    }
}

} // namespace CorePulse