#include "Window.h"
#include <iostream>
#include <GL/gl.h>

namespace CorePulse {

Window::Window(const WindowConfig& config) : config_(config) {}

Window::~Window() {
    shutdown();
}

Window::Window(Window&& other) noexcept 
    : config_(std::move(other.config_))
    , window_(other.window_)
    , gl_context_(other.gl_context_)
    , should_close_(other.should_close_) {
    other.window_ = nullptr;
    other.gl_context_ = nullptr;
}

Window& Window::operator=(Window&& other) noexcept {
    if (this != &other) {
        shutdown();
        config_ = std::move(other.config_);
        window_ = other.window_;
        gl_context_ = other.gl_context_;
        should_close_ = other.should_close_;
        other.window_ = nullptr;
        other.gl_context_ = nullptr;
    }
    return *this;
}

bool Window::initialize() {
    if (window_) {
        std::cout << "Window already initialized\n";
        return true;
    }
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }
    
    setup_gl_attributes();
    
    if (!create_window()) {
        SDL_Quit();
        return false;
    }
    
    if (!create_gl_context()) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
        SDL_Quit();
        return false;
    }
    
    set_vsync(config_.vsync);
    
    std::cout << "Window initialized successfully\n";
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;
    
    return true;
}

void Window::shutdown() {
    if (gl_context_) {
        SDL_GL_DeleteContext(gl_context_);
        gl_context_ = nullptr;
    }
    
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    
    SDL_Quit();
}

void Window::swap_buffers() {
    if (window_) {
        SDL_GL_SwapWindow(window_);
    }
}

void Window::set_title(const std::string& title) {
    config_.title = title;
    if (window_) {
        SDL_SetWindowTitle(window_, title.c_str());
    }
}

void Window::set_fullscreen(bool fullscreen) {
    if (!window_) return;
    
    config_.fullscreen = fullscreen;
    Uint32 flags = fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
    SDL_SetWindowFullscreen(window_, flags);
}

void Window::set_vsync(bool enable) {
    config_.vsync = enable;
    if (gl_context_) {
        SDL_GL_SetSwapInterval(enable ? 1 : 0);
    }
}

void Window::handle_resize(int width, int height) {
    config_.width = width;
    config_.height = height;
    glViewport(0, 0, width, height);
}

bool Window::create_window() {
    Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
    
    if (config_.fullscreen) {
        window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }
    
    if (config_.resizable) {
        window_flags |= SDL_WINDOW_RESIZABLE;
    }
    
    window_ = SDL_CreateWindow(
        config_.title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        config_.width,
        config_.height,
        window_flags
    );
    
    if (!window_) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return false;
    }
    
    return true;
}

bool Window::create_gl_context() {
    gl_context_ = SDL_GL_CreateContext(window_);
    if (!gl_context_) {
        std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
        return false;
    }
    
    return true;
}

void Window::setup_gl_attributes() {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, config_.opengl_major);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, config_.opengl_minor);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    
    // Enable multisampling for anti-aliasing
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
}

} // namespace CorePulse