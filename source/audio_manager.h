#pragma once
#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#define BUF_COUNT 3

#include <cstdint>
#include <unordered_map>
#include <string>
#include <3ds.h>

namespace AudioManager {

using AudioHandle = int; // 0 == invalid

// Initialize audio manager. Call once before Play().
bool Init();

// Shutdown audio manager, stop all sounds and free resources.
void Exit();

// Play an Opus file. Returns non-zero AudioHandle on success, 0 on failure.
// pitch: playback speed multiplier (1.0 = normal). Note: pitch changes both speed and pitch.
// loop:  whether to loop when EOF is reached.
// volume: 0.0..1.0 (values >1.0 allowed but may clip).
// pan: -1.0 = left, 0.0 = center, 1.0 = right.
AudioHandle Play(const char* path, float pitch = 1.0f, bool loop = false, float volume = 1.0f, float pan = 0.0f);

// Stop and free an instance (no-op for invalid handle).
bool StopAudio(AudioHandle handle);

// Control functions (return false if handle invalid)
bool SetPitch(AudioHandle handle, float pitch);
bool SetVolume(AudioHandle handle, float volume);
bool SetPan(AudioHandle handle, float pan);

// Query whether a handle is playing
bool IsPlaying(AudioHandle handle);

// Preload API

struct PreloadedAudio {
    int16_t* audioBuf = nullptr;
    ndspWaveBuf waveBufs[BUF_COUNT];
    int numSamples = 0;
    // Optionally: store decoded PCM, or keep OpusFile open if you want to loop
};

// Preload a sound file into memory for instant playback
bool LoadPreloadAudio(const char* path);

// Unload a previously preloaded sound file
void UnloadPreloadAudio(const char* path);

// Global cache for preloaded audio
extern std::unordered_map<std::string, PreloadedAudio> g_preloadedAudio;

void CleanupFinishedInstances();

} // namespace AudioManager

#endif // AUDIO_MANAGER_H
