#!/usr/bin/env python3
"""
Generate simple test audio files for the CorePulse audio system.
Creates basic collision sounds using sine waves.
"""

import numpy as np
import wave
import struct
import os

def generate_collision_sound(filename, frequency=440, duration=0.3, sample_rate=44100):
    """Generate a simple collision sound effect"""
    # Generate time array
    t = np.linspace(0, duration, int(sample_rate * duration))
    
    # Create collision sound: quick frequency sweep with decay
    start_freq = frequency * 2  # Start high
    end_freq = frequency * 0.5  # End low
    
    # Exponential frequency sweep
    freq_sweep = start_freq * np.exp(-3 * t / duration)
    
    # Generate sine wave with frequency sweep
    phase = 2 * np.pi * np.cumsum(freq_sweep) / sample_rate
    audio = np.sin(phase)
    
    # Apply exponential decay envelope
    envelope = np.exp(-5 * t / duration)
    audio *= envelope
    
    # Add some noise for texture
    noise = np.random.normal(0, 0.1, len(audio))
    audio = 0.8 * audio + 0.2 * noise
    
    # Normalize and convert to 16-bit
    audio = np.clip(audio, -1.0, 1.0)
    audio_16bit = (audio * 32767).astype(np.int16)
    
    # Create stereo version
    stereo_audio = np.zeros((len(audio_16bit), 2), dtype=np.int16)
    stereo_audio[:, 0] = audio_16bit  # Left channel
    stereo_audio[:, 1] = audio_16bit  # Right channel
    
    # Write WAV file
    with wave.open(filename, 'w') as wav_file:
        wav_file.setnchannels(2)  # Stereo
        wav_file.setsampwidth(2)  # 16-bit
        wav_file.setframerate(sample_rate)
        wav_file.writeframes(stereo_audio.tobytes())
    
    print(f"Generated {filename} ({duration:.2f}s, {frequency}Hz collision sound)")

def generate_bounce_sound(filename, frequency=220, duration=0.2, sample_rate=44100):
    """Generate a bounce sound effect"""
    t = np.linspace(0, duration, int(sample_rate * duration))
    
    # Bounce sound: quick high ping followed by low thump
    ping_duration = 0.05
    thump_start = 0.06
    
    audio = np.zeros(len(t))
    
    # High frequency ping at the beginning
    ping_mask = t < ping_duration
    if np.any(ping_mask):
        ping_freq = frequency * 4
        audio[ping_mask] = 0.7 * np.sin(2 * np.pi * ping_freq * t[ping_mask]) * np.exp(-20 * t[ping_mask])
    
    # Low frequency thump
    thump_mask = t >= thump_start
    if np.any(thump_mask):
        thump_t = t[thump_mask] - thump_start
        thump_freq = frequency * 0.8
        thump_audio = 0.5 * np.sin(2 * np.pi * thump_freq * thump_t) * np.exp(-8 * thump_t)
        audio[thump_mask] += thump_audio
    
    # Normalize and convert to 16-bit
    audio = np.clip(audio, -1.0, 1.0)
    audio_16bit = (audio * 32767).astype(np.int16)
    
    # Create stereo version
    stereo_audio = np.zeros((len(audio_16bit), 2), dtype=np.int16)
    stereo_audio[:, 0] = audio_16bit
    stereo_audio[:, 1] = audio_16bit
    
    # Write WAV file
    with wave.open(filename, 'w') as wav_file:
        wav_file.setnchannels(2)
        wav_file.setsampwidth(2)
        wav_file.setframerate(sample_rate)
        wav_file.writeframes(stereo_audio.tobytes())
    
    print(f"Generated {filename} ({duration:.2f}s, {frequency}Hz bounce sound)")

def generate_ambient_sound(filename, frequency=100, duration=10.0, sample_rate=44100):
    """Generate ambient background sound"""
    t = np.linspace(0, duration, int(sample_rate * duration))
    
    # Create layered ambient sound with multiple sine waves
    audio = np.zeros(len(t))
    
    # Base low frequency drone
    audio += 0.3 * np.sin(2 * np.pi * frequency * t)
    
    # Add harmonic layers
    audio += 0.2 * np.sin(2 * np.pi * frequency * 1.5 * t)
    audio += 0.15 * np.sin(2 * np.pi * frequency * 2.0 * t)
    
    # Add some gentle modulation
    modulation = 0.1 * np.sin(2 * np.pi * 0.5 * t)  # 0.5 Hz modulation
    audio *= (1.0 + modulation)
    
    # Add subtle noise for texture
    noise = np.random.normal(0, 0.05, len(audio))
    audio += noise
    
    # Apply gentle fade in/out for seamless looping
    fade_samples = int(sample_rate * 0.5)  # 0.5 second fade
    fade_in = np.linspace(0, 1, fade_samples)
    fade_out = np.linspace(1, 0, fade_samples)
    
    audio[:fade_samples] *= fade_in
    audio[-fade_samples:] *= fade_out
    
    # Normalize
    audio = np.clip(audio, -1.0, 1.0)
    audio_16bit = (audio * 16383).astype(np.int16)  # Quieter ambient sounds
    
    # Create stereo version
    stereo_audio = np.zeros((len(audio_16bit), 2), dtype=np.int16)
    stereo_audio[:, 0] = audio_16bit
    stereo_audio[:, 1] = audio_16bit
    
    # Write WAV file
    with wave.open(filename, 'w') as wav_file:
        wav_file.setnchannels(2)
        wav_file.setsampwidth(2)
        wav_file.setframerate(sample_rate)
        wav_file.writeframes(stereo_audio.tobytes())
    
    print(f"Generated {filename} ({duration:.1f}s, {frequency}Hz ambient sound)")

def main():
    # Create assets/audio directory if it doesn't exist
    audio_dir = "../assets/audio"
    os.makedirs(audio_dir, exist_ok=True)
    
    print("Generating test audio files for CorePulse...")
    
    # Generate different collision sounds
    generate_collision_sound(f"{audio_dir}/collision_metal.wav", frequency=800, duration=0.4)
    generate_collision_sound(f"{audio_dir}/collision_soft.wav", frequency=300, duration=0.3)
    generate_bounce_sound(f"{audio_dir}/bounce.wav", frequency=240, duration=0.25)
    
    # Generate ambient sounds
    generate_ambient_sound(f"{audio_dir}/ambient_hum.wav", frequency=80, duration=5.0)
    generate_ambient_sound(f"{audio_dir}/ambient_wind.wav", frequency=120, duration=5.0)
    
    print("\nAudio files generated successfully!")
    print("Files created:")
    print(f"  {audio_dir}/collision_metal.wav - High-pitched metallic collision")
    print(f"  {audio_dir}/collision_soft.wav - Lower-pitched soft collision")
    print(f"  {audio_dir}/bounce.wav - Bounce/impact sound")
    print(f"  {audio_dir}/ambient_hum.wav - Low frequency ambient hum")
    print(f"  {audio_dir}/ambient_wind.wav - Higher frequency ambient atmosphere")

if __name__ == "__main__":
    main()