#pragma once
#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#define BUF_COUNT 3

#include <cstdint>
#include <unordered_map>
#include <string>
#include <3ds.h>

namespace AudioManager {

using AudioHandle = int;

bool Init();

void Exit();

AudioHandle Play(const char* path, float pitch = 1.0f, bool loop = false, float volume = 1.0f, float pan = 0.0f);

bool StopAudio(AudioHandle handle);

bool SetPitch(AudioHandle handle, float pitch);
bool SetVolume(AudioHandle handle, float volume);
bool SetPan(AudioHandle handle, float pan);

bool IsPlaying(AudioHandle handle);


struct PreloadedAudio {
    int16_t* audioBuf = nullptr;
    ndspWaveBuf waveBufs[BUF_COUNT];
    int numSamples = 0;
};

bool LoadPreloadAudio(const char* path);

void UnloadPreloadAudio(const char* path);

extern std::unordered_map<std::string, PreloadedAudio> g_preloadedAudio;

void CleanupFinishedInstances();
bool PreloadAudio(const char* path);

}

#endif
