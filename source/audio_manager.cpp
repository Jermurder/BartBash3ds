#include "audio_manager.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static AudioClip audioClips[MAX_AUDIO_CLIPS];
static int audioClipCount = 0;

void AudioManager_Init() {
    ndspInit();
    memset(audioClips, 0, sizeof(audioClips));
    audioClipCount = 0;
}

bool AudioManager_Load(const char* name, const char* path) {
    if (audioClipCount >= MAX_AUDIO_CLIPS) return false;

    FILE* file = fopen(path, "rb");
    if (!file) {
        printf("Failed to open CWAV file: %s\n", path);
        return false;
    }

    fseek(file, 0, SEEK_END);
    u32 size = ftell(file);
    fseek(file, 0, SEEK_SET);

    void* buffer = linearAlloc(size);
    fread(buffer, 1, size, file);
    fclose(file);

    CWAV* cwav = (CWAV*)malloc(sizeof(CWAV));
    cwavLoad(cwav, buffer, 3);
    cwav->dataBuffer = buffer;

    if (cwav->loadStatus != CWAV_SUCCESS) {
        printf("Failed to load CWAV from: %s\n", path);
        free(cwav);
        linearFree(buffer);
        return false;
    }

    strncpy(audioClips[audioClipCount].name, name, AUDIO_NAME_LEN);
    audioClips[audioClipCount].cwav = cwav;
    audioClips[audioClipCount].buffer = buffer;
    audioClipCount++;
    return true;
}

void AudioManager_Play(const char* name, int channel) {
    for (int i = 0; i < audioClipCount; i++) {
        if (strcmp(audioClips[i].name, name) == 0) {
            cwavPlay(audioClips[i].cwav, 0, channel);
            break;
        }
    }
}

void AudioManager_FreeClip(const char* name) {
    for (int i = 0; i < audioClipCount; i++) {
        if (strcmp(audioClips[i].name, name) == 0) {
            cwavFree(audioClips[i].cwav);
            free(audioClips[i].cwav);
            linearFree(audioClips[i].buffer);
            // Shift array
            for (int j = i; j < audioClipCount - 1; j++) {
                audioClips[j] = audioClips[j + 1];
            }
            audioClipCount--;
            break;
        }
    }
}

void AudioManager_FreeAll() {
    for (int i = 0; i < audioClipCount; i++) {
        cwavFree(audioClips[i].cwav);
        free(audioClips[i].cwav);
        linearFree(audioClips[i].buffer);
    }
    audioClipCount = 0;
}

void AudioManager_Exit() {
    AudioManager_FreeAll();
    ndspExit();
}
