#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace CorePulse {

// Forward declarations
class Mesh;

// Transform component - position, rotation, scale
struct Transform {
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f}; // Euler angles in degrees
    glm::vec3 scale{1.0f};
    
    Transform() = default;
    Transform(const glm::vec3& pos, const glm::vec3& rot = glm::vec3(0.0f), const glm::vec3& scl = glm::vec3(1.0f))
        : position(pos), rotation(rot), scale(scl) {}
    
    // Generate model matrix from transform
    glm::mat4 get_model_matrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        
        // Apply transformations: scale -> rotation -> translation
        model = glm::translate(model, position);
        model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)); // Yaw
        model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)); // Pitch
        model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f)); // Roll
        model = glm::scale(model, scale);
        
        return model;
    }
    
    // Utility methods
    void translate(const glm::vec3& delta) { position += delta; }
    void rotate(const glm::vec3& delta) { rotation += delta; }
    void scale_by(const glm::vec3& factor) { scale *= factor; }
    void scale_by(float factor) { scale *= factor; }
};

// Renderable component - visual representation
struct Renderable {
    std::shared_ptr<Mesh> mesh;
    glm::vec3 color{1.0f}; // RGB color multiplier
    bool visible = true;
    bool cast_shadows = true;
    bool receive_shadows = true;
    
    Renderable() = default;
    explicit Renderable(std::shared_ptr<Mesh> m, const glm::vec3& c = glm::vec3(1.0f))
        : mesh(std::move(m)), color(c) {}
};

// Velocity component - physics motion
struct Velocity {
    glm::vec3 linear{0.0f};   // Linear velocity (units/second)
    glm::vec3 angular{0.0f};  // Angular velocity (degrees/second)
    
    Velocity() = default;
    Velocity(const glm::vec3& lin, const glm::vec3& ang = glm::vec3(0.0f))
        : linear(lin), angular(ang) {}
};

// Tag component - simple identifier
struct Tag {
    std::string name;
    
    Tag() = default;
    explicit Tag(const std::string& n) : name(n) {}
    explicit Tag(const char* n) : name(n) {}
};

// Lifetime component - entities that should be destroyed after a time
struct Lifetime {
    float remaining_time; // Seconds
    
    Lifetime() = default;
    explicit Lifetime(float time) : remaining_time(time) {}
};

// Rotation component - auto-rotation behavior
struct AutoRotate {
    glm::vec3 axis{0.0f, 1.0f, 0.0f}; // Rotation axis
    float speed = 45.0f; // Degrees per second
    
    AutoRotate() = default;
    AutoRotate(const glm::vec3& ax, float sp) : axis(ax), speed(sp) {}
};

// Physics Components
struct RigidBody {
    glm::vec3 velocity{0.0f};
    glm::vec3 angular_velocity{0.0f};
    float mass = 1.0f;
    float drag = 0.1f;
    float angular_drag = 0.1f;
    bool is_kinematic = false;  // If true, not affected by physics forces
    bool use_gravity = true;
};

struct Collider {
    enum class Type { Box, Sphere, Capsule };
    
    Type type = Type::Box;
    glm::vec3 size{1.0f};  // For box: half-extents, for sphere: radius in x, for capsule: radius in x, height in y
    glm::vec3 offset{0.0f}; // Offset from transform position
    bool is_trigger = false; // If true, doesn't block movement but triggers events
};

struct Ground {
    float height = 0.0f;
    float friction = 0.8f;
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
};

// Audio components
struct AudioSourceComponent {
    std::string clip_name;
    float volume = 1.0f;
    float pitch = 1.0f;
    float max_distance = 100.0f;
    bool is_3d = true;
    bool is_looping = false;
    bool play_on_start = false;
    bool play_on_collision = false;
    uint32_t audio_source_id = 0; // Internal audio system ID
};

// Ambient audio component for environmental sounds
struct AmbientAudioComponent {
    std::string clip_name;
    float volume = 0.3f;
    float fade_distance = 50.0f;  // Distance at which ambient sound starts fading
    float max_distance = 100.0f;  // Maximum distance for ambient sound
    bool is_playing = false;
    bool auto_start = true;       // Start playing automatically
    uint32_t audio_source_id = 0; // Internal audio system ID
};

// Mission component - marks entities as part of missions
struct MissionComponent {
    std::string mission_id;
    std::string role;                      // "target", "ally", "enemy", "objective"
    std::vector<std::string> objective_ids; // Which objectives this entity is part of
    bool is_essential = false;             // Mission fails if this entity is destroyed
    std::unordered_map<std::string, std::string> properties;
    
    MissionComponent() = default;
    MissionComponent(const std::string& mission, const std::string& entity_role)
        : mission_id(mission), role(entity_role) {}
};

// Mech movement component - controls mech locomotion
struct MechMovement {
    // Movement parameters
    float max_speed = 8.0f;              // Maximum forward speed (m/s)
    float acceleration = 15.0f;          // Acceleration rate (m/s²)
    float deceleration = 20.0f;          // Deceleration rate (m/s²)
    float turn_rate = 90.0f;             // Maximum turning rate (degrees/second)
    
    // Current movement state
    glm::vec3 desired_velocity{0.0f};    // Input-driven desired movement
    float current_speed = 0.0f;          // Current forward speed
    float leg_facing = 0.0f;             // Direction legs are facing (degrees)
    float torso_rotation = 0.0f;         // Independent torso rotation (degrees)
    
    // Movement flags
    bool is_moving = false;
    bool is_turning = false;
    bool can_move = true;                // Can be disabled for damage/stunned states
    
    // Torso independence
    float max_torso_twist = 90.0f;       // Max degrees torso can twist from legs
    float torso_turn_rate = 120.0f;      // Torso rotation speed (degrees/second)
    
    MechMovement() = default;
    MechMovement(float speed, float accel, float turn_speed) 
        : max_speed(speed), acceleration(accel), turn_rate(turn_speed) {}
};

// Mech animation component - handles leg animation and poses
struct MechAnimation {
    // Animation state
    enum class State {
        Idle,
        Walking,
        Turning,
        Running
    };
    
    State current_state = State::Idle;
    
    // Walk cycle parameters
    float walk_cycle_time = 0.0f;        // Current position in walk cycle (0-1)
    float walk_cycle_speed = 2.0f;       // Cycles per second when walking
    float step_height = 0.3f;            // How high legs lift during steps
    float stride_length = 1.5f;          // Length of each step
    
    // Leg positions (relative to mech center)
    glm::vec3 left_leg_offset{-0.5f, 0.0f, 0.0f};   // Left leg base position
    glm::vec3 right_leg_offset{0.5f, 0.0f, 0.0f};   // Right leg base position
    glm::vec3 left_foot_pos{0.0f};      // Current left foot world position
    glm::vec3 right_foot_pos{0.0f};     // Current right foot world position
    
    // Animation blending
    float blend_speed = 5.0f;            // How fast to blend between states
    float idle_sway_amount = 0.02f;      // Subtle idle movement
    float idle_sway_speed = 1.0f;        // Speed of idle sway
    
    // Torso and arm animation
    float torso_bob_amount = 0.1f;       // Vertical bobbing during walk
    float arm_swing_amount = 10.0f;      // Arm swing angle (degrees)
    
    MechAnimation() = default;
};

// Mech pilot component - handles input and control
struct MechPilot {
    // Input state
    glm::vec2 movement_input{0.0f};      // WASD input (-1 to 1)
    glm::vec2 look_input{0.0f};          // Mouse look input
    bool boost_input = false;            // Shift for boost/run
    bool brake_input = false;            // Ctrl for emergency brake
    
    // Control sensitivity
    float movement_sensitivity = 1.0f;
    float look_sensitivity = 1.0f;
    float mouse_smoothing = 0.1f;        // Mouse input smoothing
    
    // Player control flags
    bool player_controlled = false;      // Is this the player's mech?
    bool input_enabled = true;           // Can receive input
    
    MechPilot() = default;
    explicit MechPilot(bool is_player) : player_controlled(is_player) {}
};

} // namespace CorePulse