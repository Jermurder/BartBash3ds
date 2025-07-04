#pragma once
#include <box2d/box2d.h>

float MetersToPixels(float m);
float PixelsToMeters(float px);

void PhysicsManager_Init();
void PhysicsManager_Update(float dt);
void PhysicsManager_SpawnPlayer(float x, float y);

b2Body* PhysicsManager_GetPlayer();
b2World* PhysicsManager_GetWorld();

void PhysicsManager_TogglePlayerFrozen();