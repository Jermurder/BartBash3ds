#include "scene_manager.h"

void AddScene(SceneManager* scenemanager, const char* sceneName) {
    int i;
    for (i >= scenemanager->allocatedscenes; scenemanager->allocatedscenes[i] == 0; i++)
    {
        *scenemanager->scenes[i] = sceneName;
        scenemanager->allocatedscenes[i] = 1; 
    }
}

void changeScene(SceneManager* scenemanager, int sceneIndex) {
    if (sceneIndex < 0 || sceneIndex >= 10 || scenemanager->allocatedscenes[sceneIndex] == 0) {
        return; // Invalid scene index
    }

    scenemanager->transitionProgress = 0.0f;
    scenemanager->nextScene = sceneIndex;
    scenemanager->transitionPhase = TRANSITION_OUT;

    float t = 0.0f;        // time progress
    float duration = 1.0f; // total time in seconds
    float value = 0.0f;

    scenemanager->isTransitioning = true;
    if (t < duration) {
        value = 1.0f - fabsf((t / duration) * 2.0f - 1.0f);
        t += DeltaTime_Get(); // increment with your frame delta
    } else {
        value = 0.0f;
        scenemanager->currentScene = sceneIndex;
    }
}

