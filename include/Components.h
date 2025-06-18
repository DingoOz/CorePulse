#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <string>

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

} // namespace CorePulse