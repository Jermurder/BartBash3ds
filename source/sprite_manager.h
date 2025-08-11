#pragma once
#include "include.h"

#define MAX_SPRITESHEETS 16

typedef struct
{
    const char *name;
    C2D_SpriteSheet sheet;
} SpriteSheetEntry;

struct SpriteManager
{
    SpriteSheetEntry entries[MAX_SPRITESHEETS];
    int count;
};

void SpriteManager_Init(SpriteManager *manager);
bool SpriteManager_Load(SpriteManager *manager, const char *name, const char *path);
C2D_SpriteSheet SpriteManager_GetSheet(SpriteManager *manager, const char *name);
void SpriteManager_Free(SpriteManager *manager);
