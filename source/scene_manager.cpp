#include <cstring> // for strncpy
#include <cmath>   // for fabsf
#include "scene_manager.h"

// Add a new scene if there's a free slot
void AddScene(SceneManager *scenemanager, const char *sceneName)
{
    for (int i = 0; i < 2; i++)
    {
        if (scenemanager->allocatedscenes[i] == 0)
        {
            // Copy sceneName safely into scenes[i].name
            // WARNING: scenes[i].name is const char*; you need to store string safely.
            // If you want to store a copy, consider changing 'name' to char array or std::string.
            // For now, just assign pointer (assumes lifetime managed elsewhere).
            scenemanager->scenes[i].name = sceneName;
            scenemanager->allocatedscenes[i] = 1;
            return;
        }
    }
    // No free slot found, could log error or ignore
}

// Initialize transition to change to sceneIndex
void changeScene(SceneManager *scenemanager, int sceneIndex)
{
    if (sceneIndex < 0 || sceneIndex >= 6 || scenemanager->allocatedscenes[sceneIndex] == 0)
    {
        return; // Invalid scene index or unallocated
    }

    scenemanager->transitionProgress = 0.0f;
    scenemanager->nextScene = sceneIndex;
    scenemanager->transitionPhase = TRANSITION_OUT;
    scenemanager->isTransitioning = true;
}

// Call this each frame with frame delta time to update transition progress
void UpdateSceneTransition(SceneManager *scenemanager, float deltaTime)
{
    if (!scenemanager->isTransitioning)
        return;

    constexpr float duration = 1.0f;

    scenemanager->transitionProgress += deltaTime;

    if (scenemanager->transitionProgress >= duration)
    {
        scenemanager->currentScene = scenemanager->nextScene;
        scenemanager->isTransitioning = false;
        scenemanager->transitionPhase = TRANSITION_NONE;
        scenemanager->transitionProgress = 0.0f;
    }
    else
    {
        // Example easing value for transition (optional)
        float t = scenemanager->transitionProgress / duration;
        float value = 1.0f - std::fabs((t * 2.0f) - 1.0f);
        // Use 'value' to render transition effects if you want
        (void)value; // silence unused warning
    }
}
