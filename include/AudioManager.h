#pragma once

#include <SDL.h>
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace CorePulse {

// Audio clip data structure
struct AudioClip {
    std::vector<uint8_t> buffer;
    uint32_t length;
    SDL_AudioSpec spec;
    std::string name;
    
    AudioClip() : length(0) {}
};

// Audio source for 3D positioned audio
struct AudioSource {
    glm::vec3 position{0.0f};
    float volume = 1.0f;
    float pitch = 1.0f;
    float max_distance = 100.0f;
    bool is_3d = true;
    bool is_looping = false;
    bool is_playing = false;
    std::string clip_name;
    uint32_t play_position = 0;
};

// Audio listener (usually the camera/player position)
struct AudioListener {
    glm::vec3 position{0.0f};
    glm::vec3 forward{0.0f, 0.0f, -1.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    float master_volume = 1.0f;
};

class AudioManager {
public:
    AudioManager();
    ~AudioManager();
    
    // Initialize/Shutdown
    bool initialize();
    void shutdown();
    
    // Audio clip management
    bool load_audio_clip(const std::string& filename, const std::string& name);
    void unload_audio_clip(const std::string& name);
    
    // Playback control
    uint32_t play_sound(const std::string& clip_name, float volume = 1.0f, bool loop = false);
    uint32_t play_sound_3d(const std::string& clip_name, const glm::vec3& position, 
                          float volume = 1.0f, bool loop = false);
    void stop_sound(uint32_t source_id);
    void stop_all_sounds();
    
    // Listener control
    void set_listener_position(const glm::vec3& position);
    void set_listener_orientation(const glm::vec3& forward, const glm::vec3& up);
    void set_master_volume(float volume);
    
    // Update (call once per frame)
    void update();
    
    // Get status
    bool is_initialized() const { return initialized_; }
    
private:
    bool initialized_ = false;
    SDL_AudioDeviceID audio_device_ = 0;
    SDL_AudioSpec audio_spec_;
    
    // Audio data
    std::unordered_map<std::string, std::shared_ptr<AudioClip>> audio_clips_;
    std::unordered_map<uint32_t, AudioSource> audio_sources_;
    AudioListener listener_;
    
    // Source management
    uint32_t next_source_id_ = 1;
    std::vector<uint32_t> finished_sources_; // For cleanup
    
    // Audio callback
    static void audio_callback(void* userdata, uint8_t* stream, int len);
    void mix_audio(uint8_t* stream, int len);
    
    // 3D audio calculations
    float calculate_3d_volume(const AudioSource& source) const;
    float calculate_3d_pan(const AudioSource& source) const;
    
    // Utility
    bool load_wav_file(const std::string& filename, AudioClip& clip);
};

} // namespace CorePulse