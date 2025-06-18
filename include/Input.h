#pragma once

#include <SDL2/SDL.h>
#include <unordered_map>
#include <unordered_set>
#include <array>

namespace CorePulse {

enum class KeyState {
    Released,
    Pressed,
    Held
};

enum class MouseButton {
    Left = SDL_BUTTON_LEFT,
    Middle = SDL_BUTTON_MIDDLE,
    Right = SDL_BUTTON_RIGHT
};

struct MouseState {
    int x = 0, y = 0;
    int delta_x = 0, delta_y = 0;
    int wheel_x = 0, wheel_y = 0;
    std::array<KeyState, 5> buttons{};
};

class Input {
public:
    Input();
    ~Input() = default;
    
    // Non-copyable but movable
    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;
    Input(Input&&) = default;
    Input& operator=(Input&&) = default;
    
    // Update input state
    void update();
    void handle_event(const SDL_Event& event);
    
    // Keyboard input
    bool is_key_pressed(SDL_Scancode key) const;
    bool is_key_held(SDL_Scancode key) const;
    bool is_key_released(SDL_Scancode key) const;
    KeyState get_key_state(SDL_Scancode key) const;
    
    // Mouse input
    bool is_mouse_button_pressed(MouseButton button) const;
    bool is_mouse_button_held(MouseButton button) const;
    bool is_mouse_button_released(MouseButton button) const;
    KeyState get_mouse_button_state(MouseButton button) const;
    
    // Mouse position and movement
    int get_mouse_x() const { return mouse_state_.x; }
    int get_mouse_y() const { return mouse_state_.y; }
    int get_mouse_delta_x() const { return mouse_state_.delta_x; }
    int get_mouse_delta_y() const { return mouse_state_.delta_y; }
    
    // Mouse wheel
    int get_wheel_x() const { return mouse_state_.wheel_x; }
    int get_wheel_y() const { return mouse_state_.wheel_y; }
    
    // Utility functions
    void set_relative_mouse_mode(bool enabled);
    bool is_relative_mouse_mode() const;
    
private:
    std::unordered_map<SDL_Scancode, KeyState> keyboard_state_;
    std::unordered_set<SDL_Scancode> keys_pressed_this_frame_;
    std::unordered_set<SDL_Scancode> keys_released_this_frame_;
    
    MouseState mouse_state_;
    std::unordered_set<MouseButton> mouse_pressed_this_frame_;
    std::unordered_set<MouseButton> mouse_released_this_frame_;
    
    void update_key_states();
    void update_mouse_states();
    void reset_frame_data();
};

} // namespace CorePulse