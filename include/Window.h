#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <memory>

namespace CorePulse {

struct WindowConfig {
    std::string title = "CorePulse";
    int width = 1024;
    int height = 768;
    bool fullscreen = false;
    bool vsync = true;
    bool resizable = true;
    int opengl_major = 4;
    int opengl_minor = 3;
};

class Window {
public:
    explicit Window(const WindowConfig& config = WindowConfig{});
    ~Window();
    
    // Non-copyable but movable
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) noexcept;
    Window& operator=(Window&&) noexcept;
    
    // Window operations
    bool initialize();
    void shutdown();
    void swap_buffers();
    void set_title(const std::string& title);
    void set_fullscreen(bool fullscreen);
    void set_vsync(bool enable);
    
    // Getters
    bool is_initialized() const { return window_ != nullptr; }
    bool should_close() const { return should_close_; }
    void set_should_close(bool close) { should_close_ = close; }
    
    int get_width() const { return config_.width; }
    int get_height() const { return config_.height; }
    float get_aspect_ratio() const { return static_cast<float>(config_.width) / config_.height; }
    
    SDL_Window* get_sdl_window() const { return window_; }
    SDL_GLContext get_gl_context() const { return gl_context_; }
    
    // Event handling
    void handle_resize(int width, int height);
    
private:
    WindowConfig config_;
    SDL_Window* window_ = nullptr;
    SDL_GLContext gl_context_ = nullptr;
    bool should_close_ = false;
    
    bool create_window();
    bool create_gl_context();
    void setup_gl_attributes();
};

} // namespace CorePulse