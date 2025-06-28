#pragma once
#include <3ds.h>
#include <citro2d.h>
#include "delta_time.h"

typedef enum {
    TRANSITION_NONE,
    TRANSITION_OUT,
    TRANSITION_IN
} TransitionPhase;

typedef struct {
    int currentScene;
    float transitionProgress;
    char* scenes[10][10];
    int allocatedscenes[10];
    bool isTransitioning;
    TransitionPhase transitionPhase;
    int nextScene;
} SceneManager;

void AddScene(SceneManager* scenemanager, const char* sceneName);
void changeScene(SceneManager* scenemanager, int sceneIndex);