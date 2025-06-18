#pragma once

#include "Application.h"
#include "Renderer.h"
#include "Camera.h"
#include "Mesh.h"
#include <sstream>
#include <memory>

namespace CorePulse {

class CorePulseApp : public Application {
public:
    CorePulseApp();
    ~CorePulseApp() override = default;

protected:
    bool on_initialize() override;
    void on_update(float delta_time) override;
    void on_render() override;
    void on_shutdown() override;
    
    // Input event handlers
    void on_key_pressed(SDL_Scancode key) override;
    void on_key_released(SDL_Scancode key) override;
    void on_mouse_button_pressed(MouseButton button) override;
    void on_mouse_button_released(MouseButton button) override;
    void on_mouse_moved(int x, int y, int dx, int dy) override;
    void on_mouse_wheel(int x, int y) override;
    void on_window_resized(int width, int height) override;

private:
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<Camera> camera_;
    
    // Test meshes
    std::unique_ptr<Mesh> cube_mesh_;
    std::unique_ptr<Mesh> sphere_mesh_;
    std::unique_ptr<Mesh> plane_mesh_;
    
    // Animation state
    float rotation_angle_ = 0.0f;
    float camera_angle_ = 0.0f;
    int current_mesh_ = 0; // 0=cube, 1=sphere, 2=plane
    
    // UI state
    bool show_info_ = true;
    bool wireframe_mode_ = false;
    
    void update_window_title();
    void update_camera_position();
    void render_scene();
};

} // namespace CorePulse