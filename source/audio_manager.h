#pragma once

#include <3ds.h>
#include <citro3d.h>
#include <cwav.h>

#define MAX_AUDIO_CLIPS 16
#define AUDIO_NAME_LEN  32

typedef struct {
    char name[AUDIO_NAME_LEN];
    CWAV* cwav;
    void* buffer;
} AudioClip;

void AudioManager_Init();
void AudioManager_Exit();

bool AudioManager_Load(const char* name, const char* path);   // load with identifier
void AudioManager_Play(const char* name, int channel);           // play by name
void AudioManager_FreeClip(const char* name);                 // free individual clip
void AudioManager_FreeAll();                                  // free all loaded clips
