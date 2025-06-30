#pragma once

#include "System.h"
#include "Components.h"
#include "Renderer.h"
#include "Camera.h"
#include <memory>

namespace CorePulse {

// Forward declarations
class World;

// Rendering system - renders all entities with Transform + Renderable components
class RenderSystem : public System {
public:
    explicit RenderSystem(std::shared_ptr<Renderer> renderer, std::shared_ptr<Camera> camera);
    
    void init() override;
    void update(float delta_time) override;
    void shutdown() override;
    
    void set_world(World* world) { world_ = world; }
    void set_camera(std::shared_ptr<Camera> camera) { camera_ = camera; }
    
private:
    std::shared_ptr<Renderer> renderer_;
    std::shared_ptr<Camera> camera_;
    World* world_ = nullptr;
};

// Movement system - updates Transform based on Velocity
class MovementSystem : public System {
public:
    MovementSystem() = default;
    
    void init() override;
    void update(float delta_time) override;
    void shutdown() override;
    
    void set_world(World* world) { world_ = world; }
    
private:
    World* world_ = nullptr;
};

// Auto-rotation system - rotates entities with AutoRotate component
class AutoRotateSystem : public System {
public:
    AutoRotateSystem() = default;
    
    void init() override;
    void update(float delta_time) override;
    void shutdown() override;
    
    void set_world(World* world) { world_ = world; }
    
private:
    World* world_ = nullptr;
};

// Lifetime system - destroys entities when their lifetime expires
class LifetimeSystem : public System {
public:
    LifetimeSystem() = default;
    
    void init() override;
    void update(float delta_time) override;
    void shutdown() override;
    
    void set_world(World* world) { world_ = world; }
    
private:
    World* world_ = nullptr;
    std::vector<Entity> entities_to_destroy_;
};

// Forward declarations
class AudioSystem;
class Terrain;

// Physics system - handles rigid body physics and collision detection
class PhysicsSystem : public System {
public:
    PhysicsSystem() = default;
    
    void init() override;
    void update(float delta_time) override;
    void shutdown() override;
    
    void set_world(World* world) { world_ = world; }
    void set_audio_system(AudioSystem* audio_system) { audio_system_ = audio_system; }
    void set_terrain(std::shared_ptr<Terrain> terrain) { terrain_ = terrain; }
    void set_gravity(const glm::vec3& gravity) { gravity_ = gravity; }
    glm::vec3 get_gravity() const { return gravity_; }
    
private:
    World* world_ = nullptr;
    AudioSystem* audio_system_ = nullptr;
    std::shared_ptr<Terrain> terrain_;
    glm::vec3 gravity_{0.0f, -9.81f, 0.0f}; // Standard Earth gravity
    
    void apply_gravity(Entity entity, RigidBody& rb, float delta_time);
    void integrate_velocity(Entity entity, Transform& transform, RigidBody& rb, float delta_time);
    void apply_drag(RigidBody& rb, float delta_time);
    void check_ground_collision(Entity entity, Transform& transform, RigidBody& rb);
    void check_terrain_collision(Entity entity, Transform& transform, RigidBody& rb);
    bool check_collision(Entity entity1, Entity entity2);
    void resolve_collision(Entity entity1, Entity entity2);
    glm::vec3 calculate_impact_velocity(Entity entity1, Entity entity2) const;
    
    // Specific collision detection methods
    bool check_sphere_sphere_collision(const glm::vec3& pos1, float radius1, 
                                       const glm::vec3& pos2, float radius2);
    bool check_aabb_aabb_collision(const glm::vec3& pos1, const glm::vec3& size1,
                                   const glm::vec3& pos2, const glm::vec3& size2);
    bool check_sphere_box_collision(const glm::vec3& sphere_pos, float sphere_radius,
                                    const glm::vec3& box_pos, const glm::vec3& box_size);
    
    // Collision resolution helpers
    bool calculate_collision_info(Entity entity1, Entity entity2,
                                  glm::vec3& collision_normal, float& penetration_depth);
    void separate_objects(Entity entity1, Entity entity2,
                          const glm::vec3& collision_normal, float penetration_depth);
};

} // namespace CorePulse