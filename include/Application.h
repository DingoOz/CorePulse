#pragma once

#include "Window.h"
#include "Input.h"
#include <memory>
#include <chrono>

namespace CorePulse {

struct ApplicationConfig {
    WindowConfig window_config;
    int target_fps = 60;
    bool limit_fps = true;
};

class Application {
public:
    explicit Application(const ApplicationConfig& config = ApplicationConfig{});
    virtual ~Application() = default;
    
    // Non-copyable but movable
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = default;
    Application& operator=(Application&&) = default;
    
    // Main application lifecycle
    bool initialize();
    void run();
    void shutdown();
    
    // Application control
    void quit() { running_ = false; }
    bool is_running() const { return running_; }
    
    // Timing
    float get_delta_time() const { return delta_time_; }
    float get_fps() const { return fps_; }
    
protected:
    // Virtual methods for derived classes to override
    virtual bool on_initialize() { return true; }
    virtual void on_update(float delta_time) {}
    virtual void on_render() {}
    virtual void on_shutdown() {}
    
    // Event handling
    virtual void on_key_pressed(SDL_Scancode key) {}
    virtual void on_key_released(SDL_Scancode key) {}
    virtual void on_mouse_button_pressed(MouseButton button) {}
    virtual void on_mouse_button_released(MouseButton button) {}
    virtual void on_mouse_moved(int x, int y, int dx, int dy) {}
    virtual void on_mouse_wheel(int x, int y) {}
    virtual void on_window_resized(int width, int height) {}
    
    // Access to core systems
    Window& get_window() { return window_; }
    const Window& get_window() const { return window_; }
    Input& get_input() { return input_; }
    const Input& get_input() const { return input_; }
    
private:
    ApplicationConfig config_;
    Window window_;
    Input input_;
    
    bool running_ = false;
    bool initialized_ = false;
    
    // Timing
    std::chrono::steady_clock::time_point last_frame_time_;
    float delta_time_ = 0.0f;
    float fps_ = 0.0f;
    float frame_time_accumulator_ = 0.0f;
    int frame_count_ = 0;
    
    void main_loop();
    void handle_events();
    void update_timing();
    void render_frame();
    void limit_frame_rate();
};

} // namespace CorePulse