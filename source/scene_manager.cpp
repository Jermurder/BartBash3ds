#include <cstring>
#include <cmath>   
#include "scene_manager.h"

void AddScene(SceneManager *scenemanager, const char *sceneName)
{
    for (int i = 0; i < 6; i++)
    {
        if (scenemanager->allocatedscenes[i] == 0)
        {
            scenemanager->scenes[i].name = sceneName;
            scenemanager->allocatedscenes[i] = 1;
            return;
        }
    }
}

void changeScene(SceneManager *scenemanager, int sceneIndex)
{
    if (scenemanager->isTransitioning)
        return;

    if (sceneIndex == scenemanager->currentScene)
        return;

    if (sceneIndex < 0 || sceneIndex >= 6 || scenemanager->allocatedscenes[sceneIndex] == 0)
        return;

    scenemanager->transitionProgress = 0.0f;
    scenemanager->nextScene = sceneIndex;
    scenemanager->transitionPhase = TRANSITION_OUT;
    scenemanager->isTransitioning = true;
}

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
        float t = scenemanager->transitionProgress / duration;
        float value = 1.0f - std::fabs((t * 2.0f) - 1.0f);

        (void)value;
    }
}
