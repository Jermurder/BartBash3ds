#pragma once
#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <cstdint>

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
void Stop(AudioHandle handle);

// Control functions (return false if handle invalid)
bool SetPitch(AudioHandle handle, float pitch);
bool SetVolume(AudioHandle handle, float volume);
bool SetPan(AudioHandle handle, float pan);

// Query whether a handle is playing
bool IsPlaying(AudioHandle handle);

} // namespace AudioManager

#endif // AUDIO_MANAGER_H