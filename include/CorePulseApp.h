#pragma once

#include "Application.h"
#include "Renderer.h"
#include "Camera.h"
#include "Mesh.h"
#include "World.h"
#include "Systems.h"
#include "AudioManager.h"
#include "AudioSystem.h"
#include "GLTFLoader.h"
#include "AssetManager.h"
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
    std::unique_ptr<AssetManager> asset_manager_;
    
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
    
    // FlightHelmet glTF resources (multiple meshes/materials)
    std::vector<std::shared_ptr<Mesh>> gltf_meshes_;
    std::vector<std::shared_ptr<Material>> gltf_materials_;
    std::vector<Entity> gltf_entities_;
    
    // Animation state
    float camera_angle_ = 0.0f;
    float camera_radius_ = 12.0f; // Further back to see both demos
    float camera_height_ = 4.0f;
    bool auto_rotate_camera_ = true;
    
    // Enhanced camera controls
    bool mouse_drag_active_ = false;
    int last_mouse_x_ = 0;
    int last_mouse_y_ = 0;
    float camera_sensitivity_ = 0.5f;
    float movement_speed_ = 5.0f; // Units per second
    
    // Free camera mode tracking
    bool free_camera_mode_ = true;
    float camera_yaw_ = 0.0f;
    float camera_pitch_ = 0.0f;
    
    // Key states for smooth movement
    bool key_w_pressed_ = false;
    bool key_a_pressed_ = false;
    bool key_s_pressed_ = false;
    bool key_d_pressed_ = false;
    bool key_q_pressed_ = false;
    bool key_e_pressed_ = false;
    
    // UI state
    bool show_info_ = true;
    bool wireframe_mode_ = false;
    
    void update_window_title();
    void update_camera_position();
    void setup_ecs_systems();
    void create_demo_entities();
    void spawn_random_entity();
    void trigger_sphere_drop();
    void test_gltf_loader();
    
    // Asset management functions
    void setup_asset_manager();
    void register_core_assets();
    void load_test_assets();
    void create_entities_from_asset(const std::string& asset_id, const glm::vec3& position = glm::vec3(0.0f));
};

} // namespace CorePulse