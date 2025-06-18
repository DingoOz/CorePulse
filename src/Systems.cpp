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
    // Check if both entities have required components
    if (!world_->has_component<Transform>(entity1) || !world_->has_component<Collider>(entity1) ||
        !world_->has_component<Transform>(entity2) || !world_->has_component<Collider>(entity2)) {
        return false;
    }
    
    const auto& transform1 = world_->get_component<Transform>(entity1);
    const auto& collider1 = world_->get_component<Collider>(entity1);
    const auto& transform2 = world_->get_component<Transform>(entity2);
    const auto& collider2 = world_->get_component<Collider>(entity2);
    
    // Skip collision check if either collider is marked as trigger
    if (collider1.is_trigger || collider2.is_trigger) {
        return false;
    }
    
    // Get world positions
    glm::vec3 pos1 = transform1.position + collider1.offset;
    glm::vec3 pos2 = transform2.position + collider2.offset;
    
    // Handle different collision type combinations
    if (collider1.type == Collider::Type::Sphere && collider2.type == Collider::Type::Sphere) {
        return check_sphere_sphere_collision(pos1, collider1.size.x, pos2, collider2.size.x);
    }
    else if (collider1.type == Collider::Type::Box && collider2.type == Collider::Type::Box) {
        return check_aabb_aabb_collision(pos1, collider1.size, pos2, collider2.size);
    }
    else if ((collider1.type == Collider::Type::Sphere && collider2.type == Collider::Type::Box) ||
             (collider1.type == Collider::Type::Box && collider2.type == Collider::Type::Sphere)) {
        
        // Ensure sphere is first parameter
        if (collider1.type == Collider::Type::Box) {
            return check_sphere_box_collision(pos2, collider2.size.x, pos1, collider1.size);
        } else {
            return check_sphere_box_collision(pos1, collider1.size.x, pos2, collider2.size);
        }
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
    const auto& collider1 = world_->get_component<Collider>(entity1);
    const auto& collider2 = world_->get_component<Collider>(entity2);
    
    // Skip kinematic bodies
    if (rb1.is_kinematic && rb2.is_kinematic) return;
    
    // Calculate collision normal and penetration depth
    glm::vec3 collision_normal;
    float penetration_depth = 0.0f;
    
    if (!calculate_collision_info(entity1, entity2, collision_normal, penetration_depth)) {
        return;
    }
    
    // Separate overlapping objects
    separate_objects(entity1, entity2, collision_normal, penetration_depth);
    
    // Calculate relative velocity
    glm::vec3 relative_velocity = rb2.velocity - rb1.velocity;
    float velocity_along_normal = glm::dot(relative_velocity, collision_normal);
    
    // Don't resolve if velocities are separating
    if (velocity_along_normal > 0) return;
    
    // Calculate restitution (bounciness) based on materials
    float restitution = 0.6f; // Default bounciness
    
    // Calculate collision impulse
    float impulse_magnitude = -(1 + restitution) * velocity_along_normal;
    
    // Handle kinematic bodies (infinite mass)
    if (rb1.is_kinematic && !rb2.is_kinematic) {
        impulse_magnitude /= (1.0f / rb2.mass);
        rb2.velocity += (impulse_magnitude * collision_normal) / rb2.mass;
    } else if (rb2.is_kinematic && !rb1.is_kinematic) {
        impulse_magnitude /= (1.0f / rb1.mass);
        rb1.velocity -= (impulse_magnitude * collision_normal) / rb1.mass;
    } else if (!rb1.is_kinematic && !rb2.is_kinematic) {
        impulse_magnitude /= (1.0f / rb1.mass + 1.0f / rb2.mass);
        
        glm::vec3 impulse = impulse_magnitude * collision_normal;
        
        // Apply impulse to velocities
        rb1.velocity -= impulse / rb1.mass;
        rb2.velocity += impulse / rb2.mass;
    }
}

bool PhysicsSystem::check_sphere_sphere_collision(const glm::vec3& pos1, float radius1, 
                                                  const glm::vec3& pos2, float radius2) {
    float distance = glm::length(pos2 - pos1);
    return distance < (radius1 + radius2);
}

bool PhysicsSystem::check_aabb_aabb_collision(const glm::vec3& pos1, const glm::vec3& size1,
                                              const glm::vec3& pos2, const glm::vec3& size2) {
    // Calculate half-extents for each box
    glm::vec3 half1 = size1 * 0.5f;
    glm::vec3 half2 = size2 * 0.5f;
    
    // Check for overlap on all three axes
    return (abs(pos1.x - pos2.x) < (half1.x + half2.x)) &&
           (abs(pos1.y - pos2.y) < (half1.y + half2.y)) &&
           (abs(pos1.z - pos2.z) < (half1.z + half2.z));
}

bool PhysicsSystem::check_sphere_box_collision(const glm::vec3& sphere_pos, float sphere_radius,
                                               const glm::vec3& box_pos, const glm::vec3& box_size) {
    // Calculate half-extents of the box
    glm::vec3 box_half = box_size * 0.5f;
    
    // Find the closest point on the box to the sphere center
    glm::vec3 box_min = box_pos - box_half;
    glm::vec3 box_max = box_pos + box_half;
    
    glm::vec3 closest_point;
    closest_point.x = std::max(box_min.x, std::min(sphere_pos.x, box_max.x));
    closest_point.y = std::max(box_min.y, std::min(sphere_pos.y, box_max.y));
    closest_point.z = std::max(box_min.z, std::min(sphere_pos.z, box_max.z));
    
    // Check if the distance from sphere center to closest point is less than radius
    float distance_squared = glm::dot(closest_point - sphere_pos, closest_point - sphere_pos);
    return distance_squared < (sphere_radius * sphere_radius);
}

bool PhysicsSystem::calculate_collision_info(Entity entity1, Entity entity2,
                                             glm::vec3& collision_normal, float& penetration_depth) {
    const auto& transform1 = world_->get_component<Transform>(entity1);
    const auto& collider1 = world_->get_component<Collider>(entity1);
    const auto& transform2 = world_->get_component<Transform>(entity2);
    const auto& collider2 = world_->get_component<Collider>(entity2);
    
    glm::vec3 pos1 = transform1.position + collider1.offset;
    glm::vec3 pos2 = transform2.position + collider2.offset;
    
    // Handle sphere-sphere collision info
    if (collider1.type == Collider::Type::Sphere && collider2.type == Collider::Type::Sphere) {
        glm::vec3 direction = pos2 - pos1;
        float distance = glm::length(direction);
        float combined_radius = collider1.size.x + collider2.size.x;
        
        if (distance < combined_radius && distance > 0.0001f) {
            collision_normal = direction / distance;
            penetration_depth = combined_radius - distance;
            return true;
        }
    }
    // Handle box-box collision info
    else if (collider1.type == Collider::Type::Box && collider2.type == Collider::Type::Box) {
        glm::vec3 direction = pos2 - pos1;
        collision_normal = glm::normalize(direction);
        
        // Simple penetration depth calculation for AABB
        glm::vec3 half1 = collider1.size * 0.5f;
        glm::vec3 half2 = collider2.size * 0.5f;
        
        glm::vec3 overlap;
        overlap.x = (half1.x + half2.x) - abs(direction.x);
        overlap.y = (half1.y + half2.y) - abs(direction.y);
        overlap.z = (half1.z + half2.z) - abs(direction.z);
        
        // Find minimum overlap axis
        if (overlap.x < overlap.y && overlap.x < overlap.z) {
            penetration_depth = overlap.x;
            collision_normal = glm::vec3(direction.x > 0 ? 1.0f : -1.0f, 0.0f, 0.0f);
        } else if (overlap.y < overlap.z) {
            penetration_depth = overlap.y;
            collision_normal = glm::vec3(0.0f, direction.y > 0 ? 1.0f : -1.0f, 0.0f);
        } else {
            penetration_depth = overlap.z;
            collision_normal = glm::vec3(0.0f, 0.0f, direction.z > 0 ? 1.0f : -1.0f);
        }
        
        return penetration_depth > 0.0f;
    }
    // Handle sphere-box collision info
    else {
        // Simplified: use direction between centers
        glm::vec3 direction = pos2 - pos1;
        float distance = glm::length(direction);
        
        if (distance > 0.0001f) {
            collision_normal = direction / distance;
            penetration_depth = 0.1f; // Simple approximation
            return true;
        }
    }
    
    return false;
}

void PhysicsSystem::separate_objects(Entity entity1, Entity entity2,
                                     const glm::vec3& collision_normal, float penetration_depth) {
    auto& rb1 = world_->get_component<RigidBody>(entity1);
    auto& rb2 = world_->get_component<RigidBody>(entity2);
    auto& transform1 = world_->get_component<Transform>(entity1);
    auto& transform2 = world_->get_component<Transform>(entity2);
    
    // Calculate separation based on mass ratios
    float total_inverse_mass = 0.0f;
    if (!rb1.is_kinematic) total_inverse_mass += 1.0f / rb1.mass;
    if (!rb2.is_kinematic) total_inverse_mass += 1.0f / rb2.mass;
    
    if (total_inverse_mass <= 0.0f) return;
    
    glm::vec3 separation = collision_normal * penetration_depth;
    
    if (!rb1.is_kinematic && !rb2.is_kinematic) {
        float mass_ratio1 = (1.0f / rb1.mass) / total_inverse_mass;
        float mass_ratio2 = (1.0f / rb2.mass) / total_inverse_mass;
        
        transform1.position -= separation * mass_ratio1;
        transform2.position += separation * mass_ratio2;
    } else if (!rb1.is_kinematic) {
        transform1.position -= separation;
    } else if (!rb2.is_kinematic) {
        transform2.position += separation;
    }
}

} // namespace CorePulse