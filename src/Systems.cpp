#include "Systems.h"
#include "World.h"
#include "Mesh.h"
#include <iostream>

namespace CorePulse {

// RenderSystem implementation
RenderSystem::RenderSystem(std::shared_ptr<Renderer> renderer, std::shared_ptr<Camera> camera)
    : renderer_(std::move(renderer)), camera_(std::move(camera)) {
}

void RenderSystem::init() {
    std::cout << "RenderSystem: Initialized" << std::endl;
}

void RenderSystem::update(float delta_time) {
    if (!renderer_ || !camera_ || !world_) return;
    
    static int debug_count = 0;
    debug_count++;
    
    // Don't call begin_frame/end_frame here - that's handled by the main render loop
    
    // Render all entities with Transform and Renderable components
    for (Entity entity : entities) {
        if (debug_count % 60 == 1) {
            std::cout << "RenderSystem: Processing entity " << entity << std::endl;
        }
        
        if (!world_->has_component<Transform>(entity) || !world_->has_component<Renderable>(entity)) {
            if (debug_count % 60 == 1) {
                std::cout << "RenderSystem: Entity " << entity << " missing Transform or Renderable" << std::endl;
            }
            continue;
        }
        
        const auto& transform = world_->get_component<Transform>(entity);
        const auto& renderable = world_->get_component<Renderable>(entity);
        
        if (!renderable.visible || !renderable.mesh || !renderable.mesh->is_valid()) {
            if (debug_count % 60 == 1) {
                std::cout << "RenderSystem: Entity " << entity << " has invalid renderable" << std::endl;
            }
            continue;
        }
        
        // Set color in shader
        auto& shader = renderer_->get_default_shader();
        shader.use();
        shader.set_vec3("u_color", renderable.color);
        shader.unuse();
        
        // Render the mesh
        glm::mat4 model_matrix = transform.get_model_matrix();
        renderer_->render_mesh(*renderable.mesh, model_matrix, *camera_);
        
        if (debug_count % 60 == 1) {
            std::cout << "RenderSystem: Successfully rendered entity " << entity << std::endl;
        }
    }
}

void RenderSystem::shutdown() {
    std::cout << "RenderSystem: Shutdown" << std::endl;
}

// MovementSystem implementation
void MovementSystem::init() {
    std::cout << "MovementSystem: Initialized" << std::endl;
}

void MovementSystem::update(float delta_time) {
    if (!world_) return;
    
    // Update position based on velocity
    for (Entity entity : entities) {
        if (!world_->has_component<Transform>(entity) || !world_->has_component<Velocity>(entity)) {
            continue;
        }
        
        auto& transform = world_->get_component<Transform>(entity);
        const auto& velocity = world_->get_component<Velocity>(entity);
        
        // Apply linear velocity
        transform.translate(velocity.linear * delta_time);
        
        // Apply angular velocity
        transform.rotate(velocity.angular * delta_time);
    }
}

void MovementSystem::shutdown() {
    std::cout << "MovementSystem: Shutdown" << std::endl;
}

// AutoRotateSystem implementation
void AutoRotateSystem::init() {
    std::cout << "AutoRotateSystem: Initialized" << std::endl;
}

void AutoRotateSystem::update(float delta_time) {
    if (!world_) return;
    
    // Auto-rotate entities
    for (Entity entity : entities) {
        if (!world_->has_component<Transform>(entity) || !world_->has_component<AutoRotate>(entity)) {
            continue;
        }
        
        auto& transform = world_->get_component<Transform>(entity);
        const auto& auto_rotate = world_->get_component<AutoRotate>(entity);
        
        // Apply rotation around the specified axis
        glm::vec3 rotation_delta = auto_rotate.axis * auto_rotate.speed * delta_time;
        transform.rotate(rotation_delta);
    }
}

void AutoRotateSystem::shutdown() {
    std::cout << "AutoRotateSystem: Shutdown" << std::endl;
}

// LifetimeSystem implementation
void LifetimeSystem::init() {
    std::cout << "LifetimeSystem: Initialized" << std::endl;
}

void LifetimeSystem::update(float delta_time) {
    if (!world_) return;
    
    entities_to_destroy_.clear();
    
    // Update lifetime and mark expired entities for destruction
    for (Entity entity : entities) {
        if (!world_->has_component<Lifetime>(entity)) {
            continue;
        }
        
        auto& lifetime = world_->get_component<Lifetime>(entity);
        lifetime.remaining_time -= delta_time;
        
        if (lifetime.remaining_time <= 0.0f) {
            entities_to_destroy_.push_back(entity);
        }
    }
    
    // Destroy expired entities
    for (Entity entity : entities_to_destroy_) {
        world_->destroy_entity(entity);
    }
}

void LifetimeSystem::shutdown() {
    std::cout << "LifetimeSystem: Shutdown" << std::endl;
}

} // namespace CorePulse