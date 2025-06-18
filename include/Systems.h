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

} // namespace CorePulse