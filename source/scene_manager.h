#pragma once

#include <3ds.h>
#include <citro2d.h>
#include "delta_time.h"

typedef enum
{
	TRANSITION_NONE,
	TRANSITION_OUT,
	TRANSITION_IN
} TransitionPhase;

struct Scene
{
	const char *name;
	// Add other members as needed
};

struct SceneManager
{
	int currentScene;
	float transitionProgress;
	bool isTransitioning;
	TransitionPhase transitionPhase;
	int nextScene;
	Scene scenes[2];
	int allocatedscenes[2];

	// C++ constructor: ONLY if compiled as C++
	SceneManager()
		: currentScene(0),
		  transitionProgress(0.0f),
		  isTransitioning(false),
		  transitionPhase(TRANSITION_NONE),
		  nextScene(0),
		  scenes{{"MainMenu"}, {"Game"}},
		  allocatedscenes{1, 1} {}
};

void AddScene(SceneManager *scenemanager, const char *sceneName);
void changeScene(SceneManager *scenemanager, int sceneIndex);
