#pragma once

#include "System.h"
#include "Components.h"
#include <memory>
#include <cmath>

namespace CorePulse {

// Forward declarations
class World;
class Input;

// Mech movement and animation system
class MechMovementSystem : public System {
public:
    MechMovementSystem();
    ~MechMovementSystem() = default;
    
    void init() override;
    void update(float delta_time) override;
    void shutdown() override;
    
    void set_world(World* world) { world_ = world; }
    void set_input(Input* input) { input_ = input; }
    
    // Mech control methods
    void set_player_mech(Entity mech_entity);
    Entity get_player_mech() const { return player_mech_; }
    
    // Movement utilities
    static glm::vec3 calculate_forward_vector(float rotation_degrees);
    static glm::vec3 calculate_right_vector(float rotation_degrees);
    static float normalize_angle(float angle_degrees);
    static float angle_difference(float target, float current);
    
private:
    World* world_ = nullptr;
    Input* input_ = nullptr;
    Entity player_mech_ = 0;
    
    // Input smoothing
    glm::vec2 smoothed_movement_input_{0.0f};
    glm::vec2 smoothed_look_input_{0.0f};
    
    // Update phases
    void process_input(float delta_time);
    void update_movement(float delta_time);
    void update_animations(float delta_time);
    void apply_physics(float delta_time);
    
    // Input processing
    void process_player_input(Entity entity, MechPilot& pilot, float delta_time);
    void smooth_input(glm::vec2& target, glm::vec2& current, float smoothing, float delta_time);
    
    // Movement processing
    void process_mech_movement(Entity entity, MechMovement& movement, 
                              const MechPilot& pilot, float delta_time);
    void update_leg_facing(MechMovement& movement, float delta_time);
    void update_torso_rotation(MechMovement& movement, const MechPilot& pilot, float delta_time);
    void apply_movement_to_transform(Entity entity, const MechMovement& movement, float delta_time);
    
    // Animation processing
    void process_mech_animation(Entity entity, MechAnimation& animation, 
                               const MechMovement& movement, float delta_time);
    void update_walk_cycle(MechAnimation& animation, const MechMovement& movement, float delta_time);
    void calculate_leg_positions(MechAnimation& animation, const Transform& transform, 
                                const MechMovement& movement);
    void update_animation_state(MechAnimation& animation, const MechMovement& movement);
    
    // Physics integration
    void apply_ground_collision(Entity entity, Transform& transform, RigidBody& rb);
    void apply_mech_physics_constraints(Entity entity, MechMovement& movement, 
                                       Transform& transform, RigidBody& rb);
    
    // Utility functions
    float calculate_movement_speed(const glm::vec2& input, float max_speed, bool boost);
    glm::vec3 calculate_desired_velocity(const glm::vec2& input, float leg_rotation, float speed);
    float calculate_turning_input(const glm::vec2& movement_input);
    
    // Animation math helpers
    float calculate_step_offset(float cycle_time, bool is_left_leg);
    float calculate_step_height(float step_phase, float max_height);
    glm::vec3 calculate_foot_position(const glm::vec3& base_pos, const Transform& transform,
                                     float step_offset, float step_height, float stride_length);
    
    // Constants
    static constexpr float MOVEMENT_DEADZONE = 0.1f;
    static constexpr float ROTATION_DEADZONE = 0.05f;
    static constexpr float GROUND_CHECK_DISTANCE = 10.0f;
    static constexpr float MECH_HEIGHT_OFFSET = 1.0f;
    static constexpr float MAX_SLOPE_ANGLE = 45.0f; // Maximum walkable slope in degrees
};

} // namespace CorePulse