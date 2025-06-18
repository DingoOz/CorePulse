#include "AudioSystem.h"
#include "World.h"
#include <iostream>

namespace CorePulse {

AudioSystem::AudioSystem(std::shared_ptr<AudioManager> audio_manager)
    : audio_manager_(std::move(audio_manager)) {
}

void AudioSystem::init() {
    std::cout << "AudioSystem: Initialized" << std::endl;
}

void AudioSystem::update(float delta_time) {
    if (!world_ || !audio_manager_ || !audio_manager_->is_initialized()) return;
    
    // Process all audio sources
    process_audio_sources();
    
    // Update 3D positions for active audio sources
    update_3d_positions();
    
    // Update the audio manager
    audio_manager_->update();
}

void AudioSystem::shutdown() {
    std::cout << "AudioSystem: Shutdown" << std::endl;
}

void AudioSystem::play_audio_source(Entity entity) {
    if (!world_ || !audio_manager_) return;
    
    if (!world_->has_component<AudioSourceComponent>(entity)) {
        std::cerr << "AudioSystem: Entity " << entity << " has no AudioSourceComponent" << std::endl;
        return;
    }
    
    auto& audio_src = world_->get_component<AudioSourceComponent>(entity);
    
    // Don't start if already playing
    if (audio_src.audio_source_id != 0) return;
    
    glm::vec3 position(0.0f);
    if (world_->has_component<Transform>(entity)) {
        const auto& transform = world_->get_component<Transform>(entity);
        position = transform.position;
    }
    
    // Play the sound
    uint32_t source_id;
    if (audio_src.is_3d) {
        source_id = audio_manager_->play_sound_3d(audio_src.clip_name, position, 
                                                  audio_src.volume, audio_src.is_looping);
    } else {
        source_id = audio_manager_->play_sound(audio_src.clip_name, 
                                              audio_src.volume, audio_src.is_looping);
    }
    
    audio_src.audio_source_id = source_id;
    
    std::cout << "AudioSystem: Started playing '" << audio_src.clip_name 
              << "' for entity " << entity << " (source ID: " << source_id << ")" << std::endl;
}

void AudioSystem::stop_audio_source(Entity entity) {
    if (!world_ || !audio_manager_) return;
    
    if (!world_->has_component<AudioSourceComponent>(entity)) return;
    
    auto& audio_src = world_->get_component<AudioSourceComponent>(entity);
    
    if (audio_src.audio_source_id != 0) {
        audio_manager_->stop_sound(audio_src.audio_source_id);
        audio_src.audio_source_id = 0;
        
        std::cout << "AudioSystem: Stopped audio for entity " << entity << std::endl;
    }
}

void AudioSystem::set_listener_to_camera(const glm::vec3& camera_pos, const glm::vec3& camera_forward, const glm::vec3& camera_up) {
    if (!audio_manager_) return;
    
    audio_manager_->set_listener_position(camera_pos);
    audio_manager_->set_listener_orientation(camera_forward, camera_up);
}

void AudioSystem::trigger_collision_audio(Entity entity) {
    if (!world_ || !world_->has_component<AudioSourceComponent>(entity)) return;
    
    const auto& audio_src = world_->get_component<AudioSourceComponent>(entity);
    
    if (audio_src.play_on_collision && !audio_src.clip_name.empty()) {
        // Play collision sound (non-looping, one-shot)
        glm::vec3 position(0.0f);
        if (world_->has_component<Transform>(entity)) {
            const auto& transform = world_->get_component<Transform>(entity);
            position = transform.position;
        }
        
        if (audio_src.is_3d) {
            audio_manager_->play_sound_3d(audio_src.clip_name, position, audio_src.volume, false);
        } else {
            audio_manager_->play_sound(audio_src.clip_name, audio_src.volume, false);
        }
        
        std::cout << "AudioSystem: Triggered collision audio '" << audio_src.clip_name 
                  << "' for entity " << entity << std::endl;
    }
}

void AudioSystem::process_audio_sources() {
    // Handle entities with play_on_start flag
    for (Entity entity : entities) {
        if (!world_->has_component<AudioSourceComponent>(entity)) continue;
        
        auto& audio_src = world_->get_component<AudioSourceComponent>(entity);
        
        // Start audio if marked for play_on_start and not already playing
        if (audio_src.play_on_start && audio_src.audio_source_id == 0 && !audio_src.clip_name.empty()) {
            play_audio_source(entity);
            audio_src.play_on_start = false; // Only play once
        }
    }
}

void AudioSystem::update_3d_positions() {
    // Update positions for active 3D audio sources
    for (Entity entity : entities) {
        if (!world_->has_component<AudioSourceComponent>(entity) || 
            !world_->has_component<Transform>(entity)) continue;
        
        const auto& audio_src = world_->get_component<AudioSourceComponent>(entity);
        const auto& transform = world_->get_component<Transform>(entity);
        
        // Note: For a complete implementation, we would need to extend AudioManager
        // to support updating source positions dynamically. For now, this is a placeholder.
        // In practice, short sound effects don't need position updates, but looping
        // ambient sounds would benefit from this feature.
    }
}

} // namespace CorePulse