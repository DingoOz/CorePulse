#include "Application.h"
#include <iostream>
#include <thread>
#include <GL/gl.h>

namespace CorePulse {

Application::Application(const ApplicationConfig& config) 
    : config_(config), window_(config.window_config) {
}

bool Application::initialize() {
    if (initialized_) {
        std::cout << "Application already initialized\n";
        return true;
    }
    
    std::cout << "Initializing CorePulse Application...\n";
    
    if (!window_.initialize()) {
        std::cerr << "Failed to initialize window\n";
        return false;
    }
    
    // Initialize OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    // Call derived class initialization
    if (!on_initialize()) {
        std::cerr << "Derived class initialization failed\n";
        shutdown();
        return false;
    }
    
    initialized_ = true;
    running_ = true;
    last_frame_time_ = std::chrono::steady_clock::now();
    
    std::cout << "Application initialized successfully\n";
    return true;
}

void Application::run() {
    if (!initialized_) {
        std::cerr << "Application not initialized. Call initialize() first.\n";
        return;
    }
    
    std::cout << "Starting main loop...\n";
    main_loop();
    std::cout << "Main loop ended\n";
}

void Application::shutdown() {
    if (!initialized_) return;
    
    std::cout << "Shutting down application...\n";
    
    running_ = false;
    
    // Call derived class shutdown
    on_shutdown();
    
    window_.shutdown();
    initialized_ = false;
    
    std::cout << "Application shutdown complete\n";
}

void Application::main_loop() {
    while (running_ && !window_.should_close()) {
        handle_events();
        update_timing();
        
        input_.update();
        on_update(delta_time_);
        
        render_frame();
        window_.swap_buffers();
        
        if (config_.limit_fps) {
            limit_frame_rate();
        }
    }
}

void Application::handle_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        input_.handle_event(event);
        
        switch (event.type) {
            case SDL_QUIT:
                running_ = false;
                break;
                
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    int width = event.window.data1;
                    int height = event.window.data2;
                    window_.handle_resize(width, height);
                    on_window_resized(width, height);
                }
                break;
                
            case SDL_KEYDOWN:
                if (!event.key.repeat) {
                    on_key_pressed(event.key.keysym.scancode);
                    
                    // Handle common key combinations
                    if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                        running_ = false;
                    }
                    if (event.key.keysym.scancode == SDL_SCANCODE_F11) {
                        static bool fullscreen = false;
                        fullscreen = !fullscreen;
                        window_.set_fullscreen(fullscreen);
                    }
                }
                break;
                
            case SDL_KEYUP:
                on_key_released(event.key.keysym.scancode);
                break;
                
            case SDL_MOUSEBUTTONDOWN: {
                auto button = static_cast<MouseButton>(event.button.button);
                on_mouse_button_pressed(button);
                break;
            }
            
            case SDL_MOUSEBUTTONUP: {
                auto button = static_cast<MouseButton>(event.button.button);
                on_mouse_button_released(button);
                break;
            }
            
            case SDL_MOUSEMOTION:
                on_mouse_moved(event.motion.x, event.motion.y, 
                              event.motion.xrel, event.motion.yrel);
                break;
                
            case SDL_MOUSEWHEEL:
                on_mouse_wheel(event.wheel.x, event.wheel.y);
                break;
        }
    }
}

void Application::update_timing() {
    auto current_time = std::chrono::steady_clock::now();
    auto frame_duration = current_time - last_frame_time_;
    last_frame_time_ = current_time;
    
    delta_time_ = std::chrono::duration<float>(frame_duration).count();
    
    // Update FPS calculation
    frame_time_accumulator_ += delta_time_;
    frame_count_++;
    
    if (frame_time_accumulator_ >= 1.0f) {
        fps_ = static_cast<float>(frame_count_) / frame_time_accumulator_;
        frame_time_accumulator_ = 0.0f;
        frame_count_ = 0;
    }
}

void Application::render_frame() {
    // Clear the screen
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Call derived class rendering
    on_render();
}

void Application::limit_frame_rate() {
    if (config_.target_fps <= 0) return;
    
    float target_frame_time = 1.0f / config_.target_fps;
    float sleep_time = target_frame_time - delta_time_;
    
    if (sleep_time > 0.0f) {
        auto sleep_duration = std::chrono::duration<float>(sleep_time);
        std::this_thread::sleep_for(sleep_duration);
    }
}

} // namespace CorePulse