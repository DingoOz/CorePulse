#pragma once

#include "Application.h"
#include "Renderer.h"
#include "Camera.h"
#include "Mesh.h"
#include "World.h"
#include "Systems.h"
#include "AudioManager.h"
#include "AudioSystem.h"
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
    // Core systems
    std::shared_ptr<Renderer> renderer_;
    std::shared_ptr<Camera> camera_;
    std::unique_ptr<World> world_;
    
    // ECS systems
    std::shared_ptr<RenderSystem> render_system_;
    std::shared_ptr<MovementSystem> movement_system_;
    std::shared_ptr<AutoRotateSystem> auto_rotate_system_;
    std::shared_ptr<LifetimeSystem> lifetime_system_;
    std::shared_ptr<PhysicsSystem> physics_system_;
    std::shared_ptr<AudioSystem> audio_system_;
    
    // Audio system
    std::shared_ptr<AudioManager> audio_manager_;
    
    // Test meshes
    std::shared_ptr<Mesh> cube_mesh_;
    std::shared_ptr<Mesh> sphere_mesh_;
    std::shared_ptr<Mesh> plane_mesh_;
    
    // Demo entities
    std::vector<Entity> demo_entities_;
    Entity sphere_entity_ = 0; // Track the falling sphere for reset
    
    // Animation state
    float camera_angle_ = 0.0f;
    float camera_radius_ = 8.0f;
    float camera_height_ = 2.0f;
    bool auto_rotate_camera_ = true;
    
    // UI state
    bool show_info_ = true;
    bool wireframe_mode_ = false;
    
    void update_window_title();
    void update_camera_position();
    void setup_ecs_systems();
    void create_demo_entities();
    void spawn_random_entity();
    void trigger_sphere_drop();
};

} // namespace CorePulse