#pragma once

#include "System.h"
#include "Components.h"
#include "AudioManager.h"
#include <memory>

namespace CorePulse {

// Forward declarations
class World;

// Audio system - manages AudioSourceComponent entities and interfaces with AudioManager
class AudioSystem : public System {
public:
    explicit AudioSystem(std::shared_ptr<AudioManager> audio_manager);
    
    void init() override;
    void update(float delta_time) override;
    void shutdown() override;
    
    void set_world(World* world) { world_ = world; }
    
    // Audio control methods
    void play_audio_source(Entity entity);
    void stop_audio_source(Entity entity);
    void set_listener_to_camera(const glm::vec3& camera_pos, const glm::vec3& camera_forward, const glm::vec3& camera_up);
    
    // Collision audio support
    void trigger_collision_audio(Entity entity);
    
private:
    World* world_ = nullptr;
    std::shared_ptr<AudioManager> audio_manager_;
    
    void process_audio_sources();
    void update_3d_positions();
};

} // namespace CorePulse