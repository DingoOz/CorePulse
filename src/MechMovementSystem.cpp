#include "MechMovementSystem.h"
#include "World.h"
#include "Input.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <SDL2/SDL.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace CorePulse {

MechMovementSystem::MechMovementSystem() {
    // Constructor
}

void MechMovementSystem::init() {
    std::cout << "MechMovementSystem: Initialized" << std::endl;
}

void MechMovementSystem::update(float delta_time) {
    if (!world_) return;
    
    // Process input for player-controlled mechs
    process_input(delta_time);
    
    // Update movement logic for all mechs
    update_movement(delta_time);
    
    // Update animations
    update_animations(delta_time);
    
    // Apply physics constraints
    apply_physics(delta_time);
}

void MechMovementSystem::shutdown() {
    std::cout << "MechMovementSystem: Shutdown" << std::endl;
}

void MechMovementSystem::set_player_mech(Entity mech_entity) {
    player_mech_ = mech_entity;
    std::cout << "MechMovementSystem: Set player mech to entity " << mech_entity << std::endl;
}

void MechMovementSystem::process_input(float delta_time) {
    if (!input_ || player_mech_ == 0) return;
    
    // Find the player mech and process its input
    if (world_->has_component<MechPilot>(player_mech_) && 
        world_->has_component<MechMovement>(player_mech_)) {
        
        auto& pilot = world_->get_component<MechPilot>(player_mech_);
        if (pilot.player_controlled && pilot.input_enabled) {
            process_player_input(player_mech_, pilot, delta_time);
        }
    }
}

void MechMovementSystem::process_player_input(Entity entity, MechPilot& pilot, float delta_time) {
    if (!input_) return;
    
    // Get movement input (WASD)
    glm::vec2 raw_movement{0.0f};
    if (input_->is_key_pressed(SDL_SCANCODE_W)) raw_movement.y += 1.0f;
    if (input_->is_key_pressed(SDL_SCANCODE_S)) raw_movement.y -= 1.0f;
    if (input_->is_key_pressed(SDL_SCANCODE_A)) raw_movement.x -= 1.0f;
    if (input_->is_key_pressed(SDL_SCANCODE_D)) raw_movement.x += 1.0f;
    
    // Get boost and brake input
    pilot.boost_input = input_->is_key_pressed(SDL_SCANCODE_LSHIFT);
    pilot.brake_input = input_->is_key_pressed(SDL_SCANCODE_LCTRL);
    
    // Get mouse look input
    glm::vec2 raw_look{static_cast<float>(input_->get_mouse_delta_x()), 
                       static_cast<float>(input_->get_mouse_delta_y())};
    
    // Apply sensitivity
    raw_movement *= pilot.movement_sensitivity;
    raw_look *= pilot.look_sensitivity;
    
    // Smooth the input
    smooth_input(raw_movement, smoothed_movement_input_, pilot.mouse_smoothing, delta_time);
    smooth_input(raw_look, smoothed_look_input_, pilot.mouse_smoothing, delta_time);
    
    // Store smoothed input
    pilot.movement_input = smoothed_movement_input_;
    pilot.look_input = smoothed_look_input_;
}

void MechMovementSystem::smooth_input(glm::vec2& target, glm::vec2& current, float smoothing, float delta_time) {
    float lerp_factor = 1.0f - std::pow(smoothing, delta_time);
    current = glm::mix(current, target, lerp_factor);
}

void MechMovementSystem::update_movement(float delta_time) {
    if (!world_) return;
    
    // Iterate through all entities with MechMovement and MechPilot components
    auto& entity_manager = world_->get_entity_manager();
    
    for (Entity entity = 1; entity <= entity_manager.get_living_entity_count(); ++entity) {
        if (!world_->is_valid_entity(entity)) continue;
        
        if (world_->has_component<MechMovement>(entity) && 
            world_->has_component<MechPilot>(entity)) {
            
            auto& movement = world_->get_component<MechMovement>(entity);
            const auto& pilot = world_->get_component<MechPilot>(entity);
            
            if (movement.can_move) {
                process_mech_movement(entity, movement, pilot, delta_time);
            }
        }
    }
}

void MechMovementSystem::process_mech_movement(Entity entity, MechMovement& movement, 
                                              const MechPilot& pilot, float delta_time) {
    
    // Calculate desired speed based on input
    float desired_speed = calculate_movement_speed(pilot.movement_input, movement.max_speed, pilot.boost_input);
    
    // Apply emergency brake
    if (pilot.brake_input) {
        desired_speed = 0.0f;
    }
    
    // Update current speed with acceleration/deceleration
    if (desired_speed > movement.current_speed) {
        movement.current_speed = std::min(desired_speed, 
            movement.current_speed + movement.acceleration * delta_time);
    } else {
        float decel = pilot.brake_input ? movement.deceleration * 3.0f : movement.deceleration;
        movement.current_speed = std::max(desired_speed, 
            movement.current_speed - decel * delta_time);
    }
    
    // Update movement flags
    movement.is_moving = movement.current_speed > 0.1f;
    
    // Calculate turning input
    float turn_input = calculate_turning_input(pilot.movement_input);
    movement.is_turning = std::abs(turn_input) > ROTATION_DEADZONE;
    
    // Update leg facing direction
    if (movement.is_moving || movement.is_turning) {
        update_leg_facing(movement, delta_time);
    }
    
    // Update independent torso rotation
    update_torso_rotation(movement, pilot, delta_time);
    
    // Calculate desired velocity
    movement.desired_velocity = calculate_desired_velocity(
        pilot.movement_input, movement.leg_facing, movement.current_speed);
    
    // Apply movement to transform
    apply_movement_to_transform(entity, movement, delta_time);
}

void MechMovementSystem::update_leg_facing(MechMovement& movement, float delta_time) {
    // This is simplified - in a real implementation, this would be more complex
    // For now, legs face the direction of movement
    
    if (movement.is_moving) {
        // Legs gradually turn to face movement direction
        // This is a placeholder - would need proper pathfinding logic
        float max_turn = movement.turn_rate * delta_time;
        
        // For simplicity, we'll have legs face forward when moving
        // In a real implementation, this would consider movement direction
    }
}

void MechMovementSystem::update_torso_rotation(MechMovement& movement, const MechPilot& pilot, float delta_time) {
    // Update torso rotation based on mouse look
    if (std::abs(pilot.look_input.x) > ROTATION_DEADZONE) {
        float torso_delta = pilot.look_input.x * movement.torso_turn_rate * delta_time;
        movement.torso_rotation += torso_delta;
        
        // Clamp torso twist relative to legs
        float relative_twist = movement.torso_rotation - movement.leg_facing;
        relative_twist = normalize_angle(relative_twist);
        
        if (std::abs(relative_twist) > movement.max_torso_twist) {
            if (relative_twist > 0) {
                movement.torso_rotation = movement.leg_facing + movement.max_torso_twist;
            } else {
                movement.torso_rotation = movement.leg_facing - movement.max_torso_twist;
            }
        }
        
        movement.torso_rotation = normalize_angle(movement.torso_rotation);
    }
}

void MechMovementSystem::apply_movement_to_transform(Entity entity, const MechMovement& movement, float delta_time) {
    if (!world_->has_component<Transform>(entity)) return;
    
    auto& transform = world_->get_component<Transform>(entity);
    
    // Apply movement velocity
    if (glm::length(movement.desired_velocity) > 0.001f) {
        transform.position += movement.desired_velocity * delta_time;
    }
    
    // Update transform rotation to match leg facing
    transform.rotation.y = movement.leg_facing;
    
    // Note: The torso rotation would be applied to child transforms in a hierarchical system
    // For now, we're using the leg facing for the main transform
}

void MechMovementSystem::update_animations(float delta_time) {
    if (!world_) return;
    
    auto& entity_manager = world_->get_entity_manager();
    
    for (Entity entity = 1; entity <= entity_manager.get_living_entity_count(); ++entity) {
        if (!world_->is_valid_entity(entity)) continue;
        
        if (world_->has_component<MechAnimation>(entity) && 
            world_->has_component<MechMovement>(entity) &&
            world_->has_component<Transform>(entity)) {
            
            auto& animation = world_->get_component<MechAnimation>(entity);
            const auto& movement = world_->get_component<MechMovement>(entity);
            
            process_mech_animation(entity, animation, movement, delta_time);
        }
    }
}

void MechMovementSystem::process_mech_animation(Entity entity, MechAnimation& animation, 
                                               const MechMovement& movement, float delta_time) {
    
    // Update animation state based on movement
    update_animation_state(animation, movement);
    
    // Update walk cycle
    update_walk_cycle(animation, movement, delta_time);
    
    // Calculate leg positions
    if (world_->has_component<Transform>(entity)) {
        const auto& transform = world_->get_component<Transform>(entity);
        calculate_leg_positions(animation, transform, movement);
    }
}

void MechMovementSystem::update_animation_state(MechAnimation& animation, const MechMovement& movement) {
    MechAnimation::State new_state = MechAnimation::State::Idle;
    
    if (movement.is_moving) {
        if (movement.current_speed > movement.max_speed * 0.7f) {
            new_state = MechAnimation::State::Running;
        } else {
            new_state = MechAnimation::State::Walking;
        }
    } else if (movement.is_turning) {
        new_state = MechAnimation::State::Turning;
    }
    
    animation.current_state = new_state;
}

void MechMovementSystem::update_walk_cycle(MechAnimation& animation, const MechMovement& movement, float delta_time) {
    if (animation.current_state == MechAnimation::State::Walking || 
        animation.current_state == MechAnimation::State::Running) {
        
        // Speed up cycle for running
        float cycle_speed = animation.walk_cycle_speed;
        if (animation.current_state == MechAnimation::State::Running) {
            cycle_speed *= 1.5f;
        }
        
        // Update cycle time based on movement speed
        float speed_factor = movement.current_speed / movement.max_speed;
        animation.walk_cycle_time += cycle_speed * speed_factor * delta_time;
        
        // Wrap cycle time
        if (animation.walk_cycle_time >= 1.0f) {
            animation.walk_cycle_time -= 1.0f;
        }
    } else {
        // Gradually slow down cycle when not moving
        animation.walk_cycle_time += 0.5f * delta_time;
        if (animation.walk_cycle_time >= 1.0f) {
            animation.walk_cycle_time = 0.0f;
        }
    }
}

void MechMovementSystem::calculate_leg_positions(MechAnimation& animation, const Transform& transform, 
                                                 const MechMovement& movement) {
    
    // Calculate step offsets for each leg (alternating pattern)
    float left_step_offset = calculate_step_offset(animation.walk_cycle_time, true);
    float right_step_offset = calculate_step_offset(animation.walk_cycle_time, false);
    
    // Calculate step heights
    float left_step_height = calculate_step_height(left_step_offset, animation.step_height);
    float right_step_height = calculate_step_height(right_step_offset, animation.step_height);
    
    // Calculate world positions for feet
    animation.left_foot_pos = calculate_foot_position(
        animation.left_leg_offset, transform, left_step_offset, left_step_height, animation.stride_length);
    
    animation.right_foot_pos = calculate_foot_position(
        animation.right_leg_offset, transform, right_step_offset, right_step_height, animation.stride_length);
}

void MechMovementSystem::apply_physics(float delta_time) {
    if (!world_) return;
    
    auto& entity_manager = world_->get_entity_manager();
    
    for (Entity entity = 1; entity <= entity_manager.get_living_entity_count(); ++entity) {
        if (!world_->is_valid_entity(entity)) continue;
        
        if (world_->has_component<MechMovement>(entity) && 
            world_->has_component<Transform>(entity) &&
            world_->has_component<RigidBody>(entity)) {
            
            auto& movement = world_->get_component<MechMovement>(entity);
            auto& transform = world_->get_component<Transform>(entity);
            auto& rb = world_->get_component<RigidBody>(entity);
            
            // Apply ground collision
            apply_ground_collision(entity, transform, rb);
            
            // Apply mech-specific physics constraints
            apply_mech_physics_constraints(entity, movement, transform, rb);
        }
    }
}

void MechMovementSystem::apply_ground_collision(Entity entity, Transform& transform, RigidBody& rb) {
    // Simple ground collision - keep mech above ground level
    const float ground_level = 0.0f; // This should query the terrain system
    const float mech_ground_offset = MECH_HEIGHT_OFFSET;
    
    if (transform.position.y < ground_level + mech_ground_offset) {
        transform.position.y = ground_level + mech_ground_offset;
        
        // Stop downward velocity
        if (rb.velocity.y < 0.0f) {
            rb.velocity.y = 0.0f;
        }
    }
}

void MechMovementSystem::apply_mech_physics_constraints(Entity entity, MechMovement& movement, 
                                                       Transform& transform, RigidBody& rb) {
    
    // Apply movement velocity to physics body
    if (movement.is_moving) {
        rb.velocity.x = movement.desired_velocity.x;
        rb.velocity.z = movement.desired_velocity.z;
    } else {
        // Apply friction when not moving
        rb.velocity.x *= 0.9f;
        rb.velocity.z *= 0.9f;
    }
    
    // Limit maximum velocity
    float horizontal_speed = std::sqrt(rb.velocity.x * rb.velocity.x + rb.velocity.z * rb.velocity.z);
    if (horizontal_speed > movement.max_speed) {
        float scale = movement.max_speed / horizontal_speed;
        rb.velocity.x *= scale;
        rb.velocity.z *= scale;
    }
}

// Utility function implementations

glm::vec3 MechMovementSystem::calculate_forward_vector(float rotation_degrees) {
    float radians = glm::radians(rotation_degrees);
    return glm::vec3(std::sin(radians), 0.0f, std::cos(radians));
}

glm::vec3 MechMovementSystem::calculate_right_vector(float rotation_degrees) {
    float radians = glm::radians(rotation_degrees + 90.0f);
    return glm::vec3(std::sin(radians), 0.0f, std::cos(radians));
}

float MechMovementSystem::normalize_angle(float angle_degrees) {
    while (angle_degrees > 180.0f) angle_degrees -= 360.0f;
    while (angle_degrees < -180.0f) angle_degrees += 360.0f;
    return angle_degrees;
}

float MechMovementSystem::angle_difference(float target, float current) {
    float diff = target - current;
    return normalize_angle(diff);
}

float MechMovementSystem::calculate_movement_speed(const glm::vec2& input, float max_speed, bool boost) {
    float input_magnitude = glm::length(input);
    if (input_magnitude < MOVEMENT_DEADZONE) {
        return 0.0f;
    }
    
    float speed = input_magnitude * max_speed;
    if (boost) {
        speed *= 1.5f; // Boost multiplier
    }
    
    return std::min(speed, max_speed * (boost ? 1.5f : 1.0f));
}

glm::vec3 MechMovementSystem::calculate_desired_velocity(const glm::vec2& input, float leg_rotation, float speed) {
    if (glm::length(input) < MOVEMENT_DEADZONE) {
        return glm::vec3(0.0f);
    }
    
    // Calculate movement direction relative to leg facing
    glm::vec3 forward = calculate_forward_vector(leg_rotation);
    glm::vec3 right = calculate_right_vector(leg_rotation);
    
    glm::vec3 direction = forward * input.y + right * input.x;
    direction = glm::normalize(direction);
    
    return direction * speed;
}

float MechMovementSystem::calculate_turning_input(const glm::vec2& movement_input) {
    // For now, turning is based on A/D input
    return movement_input.x;
}

float MechMovementSystem::calculate_step_offset(float cycle_time, bool is_left_leg) {
    // Alternate legs: left leg leads by half a cycle
    float offset_time = is_left_leg ? cycle_time : cycle_time + 0.5f;
    if (offset_time >= 1.0f) offset_time -= 1.0f;
    
    return offset_time;
}

float MechMovementSystem::calculate_step_height(float step_phase, float max_height) {
    // Sine wave for smooth step motion
    if (step_phase < 0.5f) {
        // Lifting phase
        return std::sin(step_phase * 2.0f * M_PI) * max_height;
    } else {
        // Ground contact phase
        return 0.0f;
    }
}

glm::vec3 MechMovementSystem::calculate_foot_position(const glm::vec3& base_pos, const Transform& transform,
                                                     float step_offset, float step_height, float stride_length) {
    
    // Calculate forward/backward offset based on step cycle
    float forward_offset = (step_offset - 0.5f) * stride_length;
    
    // Transform to world space
    glm::vec3 world_offset = base_pos;
    world_offset.z += forward_offset; // Forward/backward movement
    world_offset.y += step_height;    // Vertical lift
    
    // Apply mech rotation
    float rotation_rad = glm::radians(transform.rotation.y);
    glm::mat3 rotation_matrix = glm::mat3(
        std::cos(rotation_rad), 0.0f, std::sin(rotation_rad),
        0.0f, 1.0f, 0.0f,
        -std::sin(rotation_rad), 0.0f, std::cos(rotation_rad)
    );
    
    world_offset = rotation_matrix * world_offset;
    
    return transform.position + world_offset;
}

} // namespace CorePulse