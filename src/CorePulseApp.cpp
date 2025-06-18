#include "CorePulseApp.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace CorePulse {

CorePulseApp::CorePulseApp() : Application([](){
    ApplicationConfig config;
    config.window_config.title = "CorePulse - Phase 1 Demo";
    config.window_config.width = 1024;
    config.window_config.height = 768;
    config.target_fps = 60;
    return config;
}()) {
}

bool CorePulseApp::on_initialize() {
    std::cout << "CorePulseApp: Initialization\n";
    
    // Initialize renderer
    renderer_ = std::make_unique<Renderer>();
    if (!renderer_->initialize()) {
        std::cerr << "Failed to initialize renderer\n";
        return false;
    }
    
    // Initialize camera
    camera_ = std::make_unique<Camera>();
    camera_->set_position(glm::vec3(0.0f, 0.0f, 5.0f));
    camera_->set_target(glm::vec3(0.0f, 0.0f, 0.0f));
    camera_->set_perspective(45.0f, get_window().get_aspect_ratio(), 0.1f, 100.0f);
    
    // Create test meshes
    cube_mesh_ = std::make_unique<Mesh>(Mesh::create_cube(1.0f));
    sphere_mesh_ = std::make_unique<Mesh>(Mesh::create_sphere(1.0f, 32));
    plane_mesh_ = std::make_unique<Mesh>(Mesh::create_plane(2.0f, 2.0f));
    
    std::cout << "Controls:\n";
    std::cout << "  ESC - Quit\n";
    std::cout << "  F11 - Toggle Fullscreen\n";
    std::cout << "  I   - Toggle Info Display\n";
    std::cout << "  W   - Toggle Wireframe Mode\n";
    std::cout << "  1/2/3 - Switch between Cube/Sphere/Plane\n";
    std::cout << "  SPACE - Reset rotation\n";
    std::cout << "  Mouse wheel - Zoom camera\n";
    std::cout << "  Arrow keys - Rotate camera\n";
    
    return true;
}

void CorePulseApp::on_update(float delta_time) {
    // Update rotation
    rotation_angle_ += 45.0f * delta_time; // 45 degrees per second
    if (rotation_angle_ > 360.0f) {
        rotation_angle_ -= 360.0f;
    }
    
    // Update camera rotation
    camera_angle_ += 20.0f * delta_time; // Slow camera orbit
    if (camera_angle_ > 360.0f) {
        camera_angle_ -= 360.0f;
    }
    
    update_camera_position();
    
    // Update window title with FPS info
    if (show_info_) {
        update_window_title();
    }
}

void CorePulseApp::on_render() {
    render_scene();
}

void CorePulseApp::on_shutdown() {
    std::cout << "CorePulseApp: Shutting down\n";
    
    // Clean up resources
    cube_mesh_.reset();
    sphere_mesh_.reset();
    plane_mesh_.reset();
    camera_.reset();
    
    if (renderer_) {
        renderer_->shutdown();
        renderer_.reset();
    }
}

void CorePulseApp::on_key_pressed(SDL_Scancode key) {
    switch (key) {
        case SDL_SCANCODE_I:
            show_info_ = !show_info_;
            if (!show_info_) {
                get_window().set_title("CorePulse - OpenGL Renderer Demo");
            }
            std::cout << "Info display: " << (show_info_ ? "ON" : "OFF") << std::endl;
            break;
            
        case SDL_SCANCODE_W:
            wireframe_mode_ = !wireframe_mode_;
            if (renderer_) {
                renderer_->set_wireframe_mode(wireframe_mode_);
            }
            std::cout << "Wireframe mode: " << (wireframe_mode_ ? "ON" : "OFF") << std::endl;
            break;
            
        case SDL_SCANCODE_1:
            current_mesh_ = 0;
            std::cout << "Switched to cube\n";
            break;
            
        case SDL_SCANCODE_2:
            current_mesh_ = 1;
            std::cout << "Switched to sphere\n";
            break;
            
        case SDL_SCANCODE_3:
            current_mesh_ = 2;
            std::cout << "Switched to plane\n";
            break;
            
        case SDL_SCANCODE_SPACE:
            rotation_angle_ = 0.0f;
            camera_angle_ = 0.0f;
            std::cout << "Animation reset\n";
            break;
            
        case SDL_SCANCODE_UP:
            if (camera_) {
                camera_->move_forward(0.5f);
            }
            break;
            
        case SDL_SCANCODE_DOWN:
            if (camera_) {
                camera_->move_backward(0.5f);
            }
            break;
            
        case SDL_SCANCODE_LEFT:
            if (camera_) {
                camera_->move_left(0.5f);
            }
            break;
            
        case SDL_SCANCODE_RIGHT:
            if (camera_) {
                camera_->move_right(0.5f);
            }
            break;
            
        default:
            std::cout << "Key pressed: " << SDL_GetScancodeName(key) << std::endl;
            break;
    }
}

void CorePulseApp::on_key_released(SDL_Scancode key) {
    // std::cout << "Key released: " << SDL_GetScancodeName(key) << std::endl;
}

void CorePulseApp::on_mouse_button_pressed(MouseButton button) {
    const char* button_name = "Unknown";
    switch (button) {
        case MouseButton::Left: button_name = "Left"; break;
        case MouseButton::Middle: button_name = "Middle"; break;
        case MouseButton::Right: button_name = "Right"; break;
    }
    std::cout << "Mouse button pressed: " << button_name << std::endl;
}

void CorePulseApp::on_mouse_button_released(MouseButton button) {
    // Optional: handle mouse button release
}

void CorePulseApp::on_mouse_moved(int x, int y, int dx, int dy) {
    // Only log significant mouse movements to avoid spam
    if (abs(dx) > 5 || abs(dy) > 5) {
        // std::cout << "Mouse moved: (" << x << ", " << y << ") delta: (" << dx << ", " << dy << ")\n";
    }
}

void CorePulseApp::on_mouse_wheel(int x, int y) {
    if (camera_) {
        float zoom_speed = 0.5f;
        if (y > 0) {
            camera_->move_forward(zoom_speed);
        } else if (y < 0) {
            camera_->move_backward(zoom_speed);
        }
    }
    std::cout << "Mouse wheel: (" << x << ", " << y << ")\n";
}

void CorePulseApp::on_window_resized(int width, int height) {
    if (camera_) {
        float aspect_ratio = static_cast<float>(width) / height;
        camera_->set_perspective(camera_->get_fov(), aspect_ratio, 
                                camera_->get_near_plane(), camera_->get_far_plane());
    }
    std::cout << "Window resized: " << width << "x" << height << std::endl;
}

void CorePulseApp::update_window_title() {
    std::ostringstream title;
    const char* mesh_names[] = {"Cube", "Sphere", "Plane"};
    title << "CorePulse - OpenGL Renderer | " << mesh_names[current_mesh_] 
          << " | FPS: " << std::fixed << std::setprecision(1) << get_fps()
          << " | Delta: " << std::fixed << std::setprecision(3) << get_delta_time() * 1000.0f << "ms";
    
    if (renderer_) {
        const auto& stats = renderer_->get_stats();
        title << " | Draw calls: " << stats.draw_calls 
              << " | Triangles: " << stats.triangles_rendered;
    }
    
    get_window().set_title(title.str());
}

void CorePulseApp::update_camera_position() {
    if (!camera_) return;
    
    // Orbit camera around the origin
    float radius = 5.0f;
    float x = radius * cos(glm::radians(camera_angle_));
    float z = radius * sin(glm::radians(camera_angle_));
    
    camera_->set_position(glm::vec3(x, 0.0f, z));
    camera_->look_at(glm::vec3(0.0f, 0.0f, 0.0f));
}

void CorePulseApp::render_scene() {
    if (!renderer_ || !camera_) return;
    
    renderer_->begin_frame();
    renderer_->clear(glm::vec4(0.1f, 0.1f, 0.2f, 1.0f));
    
    // Select current mesh
    Mesh* current_mesh = nullptr;
    glm::vec3 mesh_color(0.8f, 0.8f, 0.8f);
    
    switch (current_mesh_) {
        case 0:
            current_mesh = cube_mesh_.get();
            mesh_color = glm::vec3(1.0f, 0.5f, 0.5f); // Red cube
            break;
        case 1:
            current_mesh = sphere_mesh_.get();
            mesh_color = glm::vec3(0.5f, 1.0f, 0.5f); // Green sphere
            break;
        case 2:
            current_mesh = plane_mesh_.get();
            mesh_color = glm::vec3(0.5f, 0.5f, 1.0f); // Blue plane
            break;
    }
    
    if (current_mesh && current_mesh->is_valid()) {
        // Create model matrix with rotation
        glm::mat4 model_matrix = glm::mat4(1.0f);
        model_matrix = glm::rotate(model_matrix, glm::radians(rotation_angle_), glm::vec3(0.0f, 1.0f, 0.0f));
        model_matrix = glm::rotate(model_matrix, glm::radians(rotation_angle_ * 0.5f), glm::vec3(1.0f, 0.0f, 0.0f));
        
        // Set mesh color in shader
        auto& shader = renderer_->get_default_shader();
        shader.use();
        shader.set_vec3("u_color", mesh_color);
        shader.unuse();
        
        // Render the mesh
        renderer_->render_mesh(*current_mesh, model_matrix, *camera_);
    }
    
    renderer_->end_frame();
}

} // namespace CorePulse