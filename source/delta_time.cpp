// delta_time.c
#include "delta_time.h"

static u64 lastTime = 0;
static float deltaTime = 0.0f;

void DeltaTime_Init(void)
{
    lastTime = osGetTime();
}

float DeltaTime_Get(void)
{
    return deltaTime;
}

void DeltaTime_Update(void)
{
    u64 now = osGetTime();
    deltaTime = (now - lastTime) / 1000.0f; // Convert to seconds
    lastTime = now;
}
