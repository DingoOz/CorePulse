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
    
    // Process ambient audio
    process_ambient_audio();
    
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
    // Use default velocity for backward compatibility
    trigger_collision_audio_with_velocity(entity, glm::vec3(0.0f));
}

void AudioSystem::trigger_collision_audio_with_velocity(Entity entity, const glm::vec3& impact_velocity) {
    if (!world_ || !world_->has_component<AudioSourceComponent>(entity)) return;
    
    const auto& audio_src = world_->get_component<AudioSourceComponent>(entity);
    
    if (audio_src.play_on_collision && !audio_src.clip_name.empty()) {
        // Calculate impact intensity for volume and pitch variation
        float impact_speed = glm::length(impact_velocity);
        float impact_intensity = std::clamp(impact_speed / 10.0f, 0.1f, 2.0f); // Normalize to reasonable range
        
        // Adjust volume based on impact intensity
        float dynamic_volume = audio_src.volume * (0.5f + 0.5f * impact_intensity);
        dynamic_volume = std::clamp(dynamic_volume, 0.1f, 1.0f);
        
        // Calculate pitch variation based on impact intensity
        float pitch_variation = (impact_intensity - 1.0f) * 0.3f; // ±30% pitch variation
        pitch_variation = std::clamp(pitch_variation, -0.5f, 0.5f);
        
        // Get position
        glm::vec3 position(0.0f);
        if (world_->has_component<Transform>(entity)) {
            const auto& transform = world_->get_component<Transform>(entity);
            position = transform.position;
        }
        
        if (audio_src.is_3d) {
            // Use the enhanced velocity-based audio method
            audio_manager_->play_sound_3d_velocity(audio_src.clip_name, position, 
                                                  impact_velocity, dynamic_volume, 
                                                  pitch_variation, false);
        } else {
            audio_manager_->play_sound(audio_src.clip_name, dynamic_volume, false);
        }
        
        std::cout << "AudioSystem: Triggered collision audio '" << audio_src.clip_name 
                  << "' for entity " << entity 
                  << " (impact: " << impact_speed << ", volume: " << dynamic_volume 
                  << ", pitch: " << pitch_variation << ")" << std::endl;
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

void AudioSystem::process_ambient_audio() {
    // Process all entities with AmbientAudioComponent
    for (Entity entity : entities) {
        if (!world_->has_component<AmbientAudioComponent>(entity)) continue;
        
        auto& ambient_audio = world_->get_component<AmbientAudioComponent>(entity);
        
        // Auto-start ambient sounds
        if (ambient_audio.auto_start && !ambient_audio.is_playing && !ambient_audio.clip_name.empty()) {
            start_ambient_audio(entity);
        }
        
        // Update volume based on distance to listener
        if (ambient_audio.is_playing && world_->has_component<Transform>(entity)) {
            const auto& transform = world_->get_component<Transform>(entity);
            // Calculate distance to listener (camera position would be set via set_listener_to_camera)
            // For now, we'll use a placeholder distance calculation
            float distance = 10.0f; // This would be calculated from listener position
            update_ambient_audio_volume(entity, distance);
        }
    }
}

void AudioSystem::start_ambient_audio(Entity entity) {
    if (!world_ || !world_->has_component<AmbientAudioComponent>(entity)) return;
    
    auto& ambient_audio = world_->get_component<AmbientAudioComponent>(entity);
    
    if (ambient_audio.audio_source_id != 0) return; // Already playing
    
    glm::vec3 position(0.0f);
    if (world_->has_component<Transform>(entity)) {
        const auto& transform = world_->get_component<Transform>(entity);
        position = transform.position;
    }
    
    // Start looping ambient sound
    uint32_t source_id = audio_manager_->play_sound_3d(ambient_audio.clip_name, position, 
                                                      ambient_audio.volume, true); // Loop = true
    
    ambient_audio.audio_source_id = source_id;
    ambient_audio.is_playing = true;
    
    std::cout << "AudioSystem: Started ambient audio '" << ambient_audio.clip_name 
              << "' for entity " << entity << " (source ID: " << source_id << ")" << std::endl;
}

void AudioSystem::stop_ambient_audio(Entity entity) {
    if (!world_ || !world_->has_component<AmbientAudioComponent>(entity)) return;
    
    auto& ambient_audio = world_->get_component<AmbientAudioComponent>(entity);
    
    if (ambient_audio.audio_source_id != 0) {
        audio_manager_->stop_sound(ambient_audio.audio_source_id);
        ambient_audio.audio_source_id = 0;
        ambient_audio.is_playing = false;
        
        std::cout << "AudioSystem: Stopped ambient audio for entity " << entity << std::endl;
    }
}

void AudioSystem::update_ambient_audio_volume(Entity entity, float distance) {
    if (!world_ || !world_->has_component<AmbientAudioComponent>(entity)) return;
    
    const auto& ambient_audio = world_->get_component<AmbientAudioComponent>(entity);
    
    if (ambient_audio.audio_source_id == 0) return;
    
    // Calculate volume based on distance
    float volume_factor = 1.0f;
    if (distance > ambient_audio.fade_distance) {
        float fade_range = ambient_audio.max_distance - ambient_audio.fade_distance;
        if (fade_range > 0.0f) {
            float fade_progress = (distance - ambient_audio.fade_distance) / fade_range;
            volume_factor = 1.0f - std::clamp(fade_progress, 0.0f, 1.0f);
        } else {
            volume_factor = 0.0f;
        }
    }
    
    float adjusted_volume = ambient_audio.volume * volume_factor;
    audio_manager_->update_source_volume(ambient_audio.audio_source_id, adjusted_volume);
}

void AudioSystem::update_3d_positions() {
    // Update positions for active 3D audio sources
    for (Entity entity : entities) {
        if (!world_->has_component<Transform>(entity)) continue;
        
        const auto& transform = world_->get_component<Transform>(entity);
        
        // Update AudioSourceComponent positions
        if (world_->has_component<AudioSourceComponent>(entity)) {
            const auto& audio_src = world_->get_component<AudioSourceComponent>(entity);
            if (audio_src.audio_source_id != 0) {
                audio_manager_->update_source_position(audio_src.audio_source_id, transform.position);
            }
        }
        
        // Update AmbientAudioComponent positions
        if (world_->has_component<AmbientAudioComponent>(entity)) {
            const auto& ambient_audio = world_->get_component<AmbientAudioComponent>(entity);
            if (ambient_audio.audio_source_id != 0) {
                audio_manager_->update_source_position(ambient_audio.audio_source_id, transform.position);
            }
        }
    }
}

} // namespace CorePulse