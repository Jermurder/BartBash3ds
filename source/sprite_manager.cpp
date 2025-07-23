#include "sprite_manager.h"

void SpriteManager_Init(SpriteManager *manager)
{
    manager->count = 0;
}

bool SpriteManager_Load(SpriteManager *manager, const char *name, const char *path)
{
    if (manager->count >= MAX_SPRITESHEETS)
        return false;
    C2D_SpriteSheet sheet = C2D_SpriteSheetLoad(path);
    if (!sheet)
        return false;

    manager->entries[manager->count].name = name;
    manager->entries[manager->count].sheet = sheet;
    manager->count++;
    return true;
}

C2D_SpriteSheet SpriteManager_GetSheet(SpriteManager *manager, const char *name)
{
    for (int i = 0; i < manager->count; i++)
    {
        if (strcmp(manager->entries[i].name, name) == 0)
        {
            return manager->entries[i].sheet;
        }
    }
    return NULL;
}

void SpriteManager_Free(SpriteManager *manager)
{
    for (int i = 0; i < manager->count; i++)
    {
        C2D_SpriteSheetFree(manager->entries[i].sheet);
    }
    manager->count = 0;
}
