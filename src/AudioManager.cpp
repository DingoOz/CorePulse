#include "AudioManager.h"
#include <iostream>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace CorePulse {

AudioManager::AudioManager() = default;

AudioManager::~AudioManager() {
    shutdown();
}

bool AudioManager::initialize() {
    if (initialized_) {
        std::cout << "AudioManager: Already initialized" << std::endl;
        return true;
    }
    
    std::cout << "AudioManager: Initializing SDL2 audio..." << std::endl;
    
    // Initialize SDL audio subsystem
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        std::cerr << "AudioManager: Failed to initialize SDL audio: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Set up desired audio specification
    SDL_AudioSpec desired_spec;
    SDL_zero(desired_spec);
    desired_spec.freq = 44100;        // 44.1kHz sample rate
    desired_spec.format = AUDIO_S16SYS; // 16-bit signed audio
    desired_spec.channels = 2;         // Stereo
    desired_spec.samples = 4096;       // Buffer size
    desired_spec.callback = audio_callback;
    desired_spec.userdata = this;
    
    // Open audio device
    audio_device_ = SDL_OpenAudioDevice(nullptr, 0, &desired_spec, &audio_spec_, 
                                       SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    
    if (audio_device_ == 0) {
        std::cerr << "AudioManager: Failed to open audio device: " << SDL_GetError() << std::endl;
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return false;
    }
    
    std::cout << "AudioManager: Audio device opened successfully" << std::endl;
    std::cout << "  Frequency: " << audio_spec_.freq << " Hz" << std::endl;
    std::cout << "  Format: " << audio_spec_.format << std::endl;
    std::cout << "  Channels: " << static_cast<int>(audio_spec_.channels) << std::endl;
    std::cout << "  Samples: " << audio_spec_.samples << std::endl;
    
    // Start audio playback
    SDL_PauseAudioDevice(audio_device_, 0);
    
    initialized_ = true;
    std::cout << "AudioManager: Initialization complete" << std::endl;
    return true;
}

void AudioManager::shutdown() {
    if (!initialized_) return;
    
    std::cout << "AudioManager: Shutting down..." << std::endl;
    
    // Stop all audio playback
    stop_all_sounds();
    
    // Close audio device
    if (audio_device_ != 0) {
        SDL_CloseAudioDevice(audio_device_);
        audio_device_ = 0;
    }
    
    // Clean up audio clips
    audio_clips_.clear();
    audio_sources_.clear();
    
    // Quit SDL audio subsystem
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    
    initialized_ = false;
    std::cout << "AudioManager: Shutdown complete" << std::endl;
}

bool AudioManager::load_audio_clip(const std::string& filename, const std::string& name) {
    if (!initialized_) {
        std::cerr << "AudioManager: Not initialized" << std::endl;
        return false;
    }
    
    std::cout << "AudioManager: Loading audio clip '" << name << "' from " << filename << std::endl;
    
    auto clip = std::make_shared<AudioClip>();
    clip->name = name;
    
    if (!load_wav_file(filename, *clip)) {
        std::cerr << "AudioManager: Failed to load audio file: " << filename << std::endl;
        return false;
    }
    
    audio_clips_[name] = clip;
    std::cout << "AudioManager: Successfully loaded audio clip '" << name << "'" << std::endl;
    return true;
}

void AudioManager::unload_audio_clip(const std::string& name) {
    auto it = audio_clips_.find(name);
    if (it != audio_clips_.end()) {
        std::cout << "AudioManager: Unloading audio clip '" << name << "'" << std::endl;
        audio_clips_.erase(it);
    }
}

uint32_t AudioManager::play_sound(const std::string& clip_name, float volume, bool loop) {
    return play_sound_3d(clip_name, glm::vec3(0.0f), volume, loop);
}

uint32_t AudioManager::play_sound_3d(const std::string& clip_name, const glm::vec3& position, 
                                     float volume, bool loop) {
    return play_sound_3d_velocity(clip_name, position, glm::vec3(0.0f), volume, 0.0f, loop);
}

uint32_t AudioManager::play_sound_3d_velocity(const std::string& clip_name, const glm::vec3& position,
                                             const glm::vec3& velocity, float volume, 
                                             float pitch_variation, bool loop) {
    if (!initialized_) {
        std::cerr << "AudioManager: Not initialized" << std::endl;
        return 0;
    }
    
    auto clip_it = audio_clips_.find(clip_name);
    if (clip_it == audio_clips_.end()) {
        std::cerr << "AudioManager: Audio clip '" << clip_name << "' not found" << std::endl;
        return 0;
    }
    
    uint32_t source_id = next_source_id_++;
    
    AudioSource source;
    source.position = position;
    source.velocity = velocity;
    source.volume = std::clamp(volume, 0.0f, 1.0f);
    source.base_pitch = 1.0f + pitch_variation;
    source.pitch = source.base_pitch;
    source.is_3d = true;
    source.is_looping = loop;
    source.is_playing = true;
    source.use_doppler = audio_config_.enable_doppler && (glm::length(velocity) > 0.1f);
    source.clip_name = clip_name;
    source.play_position = 0;
    
    SDL_LockAudioDevice(audio_device_);
    audio_sources_[source_id] = source;
    SDL_UnlockAudioDevice(audio_device_);
    
    std::cout << "AudioManager: Playing 3D sound '" << clip_name << "' (ID: " << source_id 
              << ", velocity: " << glm::length(velocity) << ")" << std::endl;
    return source_id;
}

void AudioManager::stop_sound(uint32_t source_id) {
    SDL_LockAudioDevice(audio_device_);
    auto it = audio_sources_.find(source_id);
    if (it != audio_sources_.end()) {
        it->second.is_playing = false;
        std::cout << "AudioManager: Stopped sound ID " << source_id << std::endl;
    }
    SDL_UnlockAudioDevice(audio_device_);
}

void AudioManager::stop_all_sounds() {
    SDL_LockAudioDevice(audio_device_);
    for (auto& [id, source] : audio_sources_) {
        source.is_playing = false;
    }
    SDL_UnlockAudioDevice(audio_device_);
    std::cout << "AudioManager: Stopped all sounds" << std::endl;
}

void AudioManager::pause_sound(uint32_t source_id) {
    SDL_LockAudioDevice(audio_device_);
    auto it = audio_sources_.find(source_id);
    if (it != audio_sources_.end()) {
        it->second.is_playing = false;
    }
    SDL_UnlockAudioDevice(audio_device_);
}

void AudioManager::resume_sound(uint32_t source_id) {
    SDL_LockAudioDevice(audio_device_);
    auto it = audio_sources_.find(source_id);
    if (it != audio_sources_.end()) {
        it->second.is_playing = true;
    }
    SDL_UnlockAudioDevice(audio_device_);
}

void AudioManager::set_listener_position(const glm::vec3& position) {
    listener_.position = position;
}

void AudioManager::set_listener_velocity(const glm::vec3& velocity) {
    listener_.velocity = velocity;
}

void AudioManager::set_listener_orientation(const glm::vec3& forward, const glm::vec3& up) {
    listener_.forward = glm::normalize(forward);
    listener_.up = glm::normalize(up);
}

void AudioManager::set_master_volume(float volume) {
    listener_.master_volume = std::clamp(volume, 0.0f, 1.0f);
}

void AudioManager::set_audio_config(const AudioConfig& config) {
    audio_config_ = config;
}

void AudioManager::update_source_position(uint32_t source_id, const glm::vec3& position) {
    SDL_LockAudioDevice(audio_device_);
    auto it = audio_sources_.find(source_id);
    if (it != audio_sources_.end()) {
        it->second.position = position;
    }
    SDL_UnlockAudioDevice(audio_device_);
}

void AudioManager::update_source_velocity(uint32_t source_id, const glm::vec3& velocity) {
    SDL_LockAudioDevice(audio_device_);
    auto it = audio_sources_.find(source_id);
    if (it != audio_sources_.end()) {
        it->second.velocity = velocity;
        it->second.use_doppler = audio_config_.enable_doppler && (glm::length(velocity) > 0.1f);
    }
    SDL_UnlockAudioDevice(audio_device_);
}

void AudioManager::update_source_volume(uint32_t source_id, float volume) {
    SDL_LockAudioDevice(audio_device_);
    auto it = audio_sources_.find(source_id);
    if (it != audio_sources_.end()) {
        it->second.volume = std::clamp(volume, 0.0f, 1.0f);
    }
    SDL_UnlockAudioDevice(audio_device_);
}

void AudioManager::update() {
    if (!initialized_) return;
    
    // Clean up finished sources
    SDL_LockAudioDevice(audio_device_);
    for (uint32_t id : finished_sources_) {
        audio_sources_.erase(id);
    }
    finished_sources_.clear();
    SDL_UnlockAudioDevice(audio_device_);
}

void AudioManager::audio_callback(void* userdata, uint8_t* stream, int len) {
    AudioManager* manager = static_cast<AudioManager*>(userdata);
    manager->mix_audio(stream, len);
}

void AudioManager::mix_audio(uint8_t* stream, int len) {
    // Clear the stream
    SDL_memset(stream, 0, len);
    
    int16_t* output = reinterpret_cast<int16_t*>(stream);
    int samples = len / sizeof(int16_t);
    
    // Mix all active audio sources
    for (auto& [id, source] : audio_sources_) {
        if (!source.is_playing) continue;
        
        auto clip_it = audio_clips_.find(source.clip_name);
        if (clip_it == audio_clips_.end()) continue;
        
        auto& clip = clip_it->second;
        
        // Calculate 3D audio parameters
        float volume = source.volume * listener_.master_volume;
        float pitch = source.pitch;
        
        if (source.is_3d) {
            // Distance attenuation
            volume *= calculate_3d_volume(source);
            
            // Doppler effect
            if (source.use_doppler && audio_config_.enable_doppler) {
                pitch *= calculate_doppler_pitch(source);
            }
            
            // Audio occlusion (simple distance-based filtering)
            if (audio_config_.enable_occlusion) {
                volume *= calculate_occlusion_factor(source);
            }
        }
        
        // Convert volume to integer (0-128 for SDL)
        int vol = static_cast<int>(volume * 128.0f);
        if (vol <= 0) continue;
        
        // Mix the audio data with pitch adjustment
        int16_t* clip_data = reinterpret_cast<int16_t*>(clip->buffer.data());
        uint32_t clip_samples = clip->length / sizeof(int16_t);
        
        // Calculate pitch step (how fast to advance through the source)
        float pitch_step = std::clamp(pitch, 0.5f, 2.0f); // Limit pitch range
        
        for (int i = 0; i < samples && source.play_position < clip_samples; i += 2) {
            // Get stereo sample from source with interpolation for pitch shifting
            uint32_t sample_pos = static_cast<uint32_t>(source.play_position);
            
            int16_t left = 0, right = 0;
            if (sample_pos < clip_samples) {
                left = clip_data[sample_pos];
                right = (audio_spec_.channels > 1 && sample_pos + 1 < clip_samples) 
                       ? clip_data[sample_pos + 1] : left;
            }
            
            // Apply volume and pan
            left = static_cast<int16_t>((left * vol) / 128);
            right = static_cast<int16_t>((right * vol) / 128);
            
            // Apply 3D panning
            if (source.is_3d) {
                float pan = calculate_3d_pan(source);
                float left_gain = 1.0f - std::max(0.0f, pan);
                float right_gain = 1.0f + std::min(0.0f, pan);
                left = static_cast<int16_t>(left * left_gain);
                right = static_cast<int16_t>(right * right_gain);
            }
            
            // Mix with existing audio
            int32_t mixed_left = output[i] + left;
            int32_t mixed_right = output[i + 1] + right;
            
            // Clamp to prevent overflow
            output[i] = static_cast<int16_t>(std::clamp(mixed_left, -32768, 32767));
            output[i + 1] = static_cast<int16_t>(std::clamp(mixed_right, -32768, 32767));
            
            // Advance source position with pitch adjustment
            source.play_position += static_cast<uint32_t>(2 * pitch_step);
        }
        
        // Check if finished playing
        if (source.play_position >= clip_samples) {
            if (source.is_looping) {
                source.play_position = 0; // Loop back to start
            } else {
                source.is_playing = false;
                finished_sources_.push_back(id);
            }
        }
    }
}

float AudioManager::calculate_3d_volume(const AudioSource& source) const {
    float distance = glm::length(source.position - listener_.position);
    
    if (distance >= source.max_distance) {
        return 0.0f;
    }
    
    if (distance <= source.min_distance) {
        return 1.0f;
    }
    
    // Logarithmic falloff with configurable rolloff factor
    float distance_ratio = (distance - source.min_distance) / (source.max_distance - source.min_distance);
    float attenuation = 1.0f / (1.0f + source.rolloff_factor * distance_ratio * distance_ratio);
    
    // Apply global distance model factor
    attenuation *= audio_config_.distance_model_factor;
    
    return std::clamp(attenuation, 0.0f, 1.0f);
}

float AudioManager::calculate_3d_pan(const AudioSource& source) const {
    glm::vec3 to_source = source.position - listener_.position;
    float distance = glm::length(to_source);
    
    if (distance < 0.001f) {
        return 0.0f; // Centered if source is at listener position
    }
    
    to_source = glm::normalize(to_source);
    glm::vec3 right = glm::cross(listener_.forward, listener_.up);
    
    // Calculate how much to the right the source is (-1 = left, 1 = right)
    float pan = glm::dot(to_source, right);
    
    // Reduce panning for distant sources (they should sound more centered)
    float distance_factor = std::min(1.0f, distance / source.max_distance);
    pan *= (1.0f - distance_factor * 0.5f);
    
    return std::clamp(pan, -1.0f, 1.0f);
}

float AudioManager::calculate_doppler_pitch(const AudioSource& source) const {
    if (!audio_config_.enable_doppler) {
        return 1.0f;
    }
    
    glm::vec3 to_source = source.position - listener_.position;
    float distance = glm::length(to_source);
    
    if (distance < 0.001f) {
        return 1.0f;
    }
    
    glm::vec3 direction = to_source / distance;
    
    // Calculate relative velocity along the line between listener and source
    float source_velocity = glm::dot(source.velocity, direction);
    float listener_velocity = glm::dot(listener_.velocity, direction);
    float relative_velocity = source_velocity - listener_velocity;
    
    // Doppler shift formula: f' = f * (v + vr) / (v + vs)
    // where v = speed of sound, vr = receiver velocity, vs = source velocity
    float doppler_factor = (audio_config_.speed_of_sound - listener_velocity) / 
                          (audio_config_.speed_of_sound - source_velocity);
    
    // Apply doppler intensity factor and clamp to reasonable range
    doppler_factor = 1.0f + (doppler_factor - 1.0f) * audio_config_.doppler_factor;
    return std::clamp(doppler_factor, 0.5f, 2.0f);
}

float AudioManager::calculate_distance_filter(const AudioSource& source) const {
    float distance = glm::length(source.position - listener_.position);
    
    if (distance <= source.min_distance) {
        return 1.0f; // No filtering at close distance
    }
    
    // Simple low-pass effect based on distance
    float filter_factor = source.min_distance / distance;
    return std::clamp(filter_factor, 0.1f, 1.0f);
}

float AudioManager::calculate_occlusion_factor(const AudioSource& source) const {
    float distance = glm::length(source.position - listener_.position);
    
    // Simple occlusion based on distance (more sophisticated methods would use raycasting)
    float occlusion = 1.0f;
    
    // Simulate simple air absorption
    if (distance > source.min_distance) {
        float air_absorption = 1.0f - (distance - source.min_distance) / (source.max_distance * 2.0f);
        occlusion *= std::max(0.2f, air_absorption);
    }
    
    return occlusion;
}

bool AudioManager::load_wav_file(const std::string& filename, AudioClip& clip) {
    SDL_AudioSpec wav_spec;
    uint8_t* wav_buffer;
    uint32_t wav_length;
    
    if (SDL_LoadWAV(filename.c_str(), &wav_spec, &wav_buffer, &wav_length) == nullptr) {
        std::cerr << "AudioManager: Failed to load WAV file: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Convert audio to our target format if needed
    SDL_AudioCVT cvt;
    int ret = SDL_BuildAudioCVT(&cvt, 
                               wav_spec.format, wav_spec.channels, wav_spec.freq,
                               audio_spec_.format, audio_spec_.channels, audio_spec_.freq);
    
    if (ret == -1) {
        std::cerr << "AudioManager: Could not build audio converter: " << SDL_GetError() << std::endl;
        SDL_FreeWAV(wav_buffer);
        return false;
    }
    
    if (ret == 0) {
        // No conversion needed
        clip.buffer.assign(wav_buffer, wav_buffer + wav_length);
        clip.length = wav_length;
    } else {
        // Conversion needed
        cvt.buf = new uint8_t[wav_length * cvt.len_mult];
        SDL_memcpy(cvt.buf, wav_buffer, wav_length);
        cvt.len = wav_length;
        
        if (SDL_ConvertAudio(&cvt) < 0) {
            std::cerr << "AudioManager: Audio conversion failed: " << SDL_GetError() << std::endl;
            delete[] cvt.buf;
            SDL_FreeWAV(wav_buffer);
            return false;
        }
        
        clip.buffer.assign(cvt.buf, cvt.buf + cvt.len_cvt);
        clip.length = cvt.len_cvt;
        
        delete[] cvt.buf;
    }
    
    clip.spec = audio_spec_;
    SDL_FreeWAV(wav_buffer);
    
    std::cout << "AudioManager: Loaded WAV file " << filename 
              << " (" << clip.length << " bytes)" << std::endl;
    return true;
}

} // namespace CorePulse