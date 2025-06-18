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

// PhysicsSystem implementation
void PhysicsSystem::init() {
    std::cout << "PhysicsSystem: Initialized" << std::endl;
}

void PhysicsSystem::update(float delta_time) {
    if (!world_) return;
    
    // Update physics for all entities with RigidBody component
    for (Entity entity : entities) {
        if (!world_->has_component<Transform>(entity) || !world_->has_component<RigidBody>(entity)) {
            continue;
        }
        
        auto& transform = world_->get_component<Transform>(entity);
        auto& rb = world_->get_component<RigidBody>(entity);
        
        // Skip kinematic bodies (they're controlled externally)
        if (rb.is_kinematic) continue;
        
        // Apply gravity
        if (rb.use_gravity) {
            apply_gravity(entity, rb, delta_time);
        }
        
        // Apply drag
        apply_drag(rb, delta_time);
        
        // Integrate velocity to update position
        integrate_velocity(entity, transform, rb, delta_time);
        
        // Check ground collision
        check_ground_collision(entity, transform, rb);
    }
    
    // Check collisions between entities
    auto entities_vec = std::vector<Entity>(entities.begin(), entities.end());
    for (size_t i = 0; i < entities_vec.size(); ++i) {
        for (size_t j = i + 1; j < entities_vec.size(); ++j) {
            if (check_collision(entities_vec[i], entities_vec[j])) {
                resolve_collision(entities_vec[i], entities_vec[j]);
            }
        }
    }
}

void PhysicsSystem::shutdown() {
    std::cout << "PhysicsSystem: Shutdown" << std::endl;
}

void PhysicsSystem::apply_gravity(Entity entity, RigidBody& rb, float delta_time) {
    rb.velocity += gravity_ * delta_time;
}

void PhysicsSystem::integrate_velocity(Entity entity, Transform& transform, RigidBody& rb, float delta_time) {
    // Update position
    transform.position += rb.velocity * delta_time;
    
    // Update rotation (convert angular velocity from degrees to radians)
    glm::vec3 angular_rad = glm::radians(rb.angular_velocity);
    transform.rotation += angular_rad * delta_time;
    
    // Keep rotation in reasonable bounds
    for (int i = 0; i < 3; ++i) {
        while (transform.rotation[i] > glm::pi<float>()) transform.rotation[i] -= 2.0f * glm::pi<float>();
        while (transform.rotation[i] < -glm::pi<float>()) transform.rotation[i] += 2.0f * glm::pi<float>();
    }
}

void PhysicsSystem::apply_drag(RigidBody& rb, float delta_time) {
    // Apply linear drag
    float drag_factor = std::max(0.0f, 1.0f - rb.drag * delta_time);
    rb.velocity *= drag_factor;
    
    // Apply angular drag
    float angular_drag_factor = std::max(0.0f, 1.0f - rb.angular_drag * delta_time);
    rb.angular_velocity *= angular_drag_factor;
}

void PhysicsSystem::check_ground_collision(Entity entity, Transform& transform, RigidBody& rb) {
    // Simple ground plane collision at y = 0
    const float ground_y = 0.0f;
    const float bounce_factor = 0.3f;
    
    if (transform.position.y < ground_y) {
        // Move back to ground level
        transform.position.y = ground_y;
        
        // Reverse and dampen vertical velocity if moving downward
        if (rb.velocity.y < 0.0f) {
            rb.velocity.y = -rb.velocity.y * bounce_factor;
        }
        
        // Apply ground friction to horizontal movement
        const float friction = 0.8f;
        rb.velocity.x *= (1.0f - friction * 0.016f); // Approximate frame time
        rb.velocity.z *= (1.0f - friction * 0.016f);
    }
}

bool PhysicsSystem::check_collision(Entity entity1, Entity entity2) {
    // For now, just check if both entities have colliders
    if (!world_->has_component<Transform>(entity1) || !world_->has_component<Collider>(entity1) ||
        !world_->has_component<Transform>(entity2) || !world_->has_component<Collider>(entity2)) {
        return false;
    }
    
    const auto& transform1 = world_->get_component<Transform>(entity1);
    const auto& collider1 = world_->get_component<Collider>(entity1);
    const auto& transform2 = world_->get_component<Transform>(entity2);
    const auto& collider2 = world_->get_component<Collider>(entity2);
    
    // Simple sphere-sphere collision for now
    if (collider1.type == Collider::Type::Sphere && collider2.type == Collider::Type::Sphere) {
        glm::vec3 pos1 = transform1.position + collider1.offset;
        glm::vec3 pos2 = transform2.position + collider2.offset;
        float radius1 = collider1.size.x;
        float radius2 = collider2.size.x;
        
        float distance = glm::length(pos2 - pos1);
        return distance < (radius1 + radius2);
    }
    
    return false;
}

void PhysicsSystem::resolve_collision(Entity entity1, Entity entity2) {
    if (!world_->has_component<RigidBody>(entity1) || !world_->has_component<RigidBody>(entity2)) {
        return;
    }
    
    auto& rb1 = world_->get_component<RigidBody>(entity1);
    auto& rb2 = world_->get_component<RigidBody>(entity2);
    auto& transform1 = world_->get_component<Transform>(entity1);
    auto& transform2 = world_->get_component<Transform>(entity2);
    
    // Simple elastic collision response
    glm::vec3 collision_normal = glm::normalize(transform2.position - transform1.position);
    
    // Calculate relative velocity
    glm::vec3 relative_velocity = rb2.velocity - rb1.velocity;
    float velocity_along_normal = glm::dot(relative_velocity, collision_normal);
    
    // Don't resolve if velocities are separating
    if (velocity_along_normal > 0) return;
    
    // Calculate collision impulse
    float restitution = 0.6f; // Bounciness factor
    float impulse_magnitude = -(1 + restitution) * velocity_along_normal;
    impulse_magnitude /= (1.0f / rb1.mass + 1.0f / rb2.mass);
    
    glm::vec3 impulse = impulse_magnitude * collision_normal;
    
    // Apply impulse to velocities
    rb1.velocity -= impulse / rb1.mass;
    rb2.velocity += impulse / rb2.mass;
}

} // namespace CorePulse