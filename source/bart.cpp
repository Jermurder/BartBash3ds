#include "bart.h"
#include "sprite_manager.h"

Bart barts[40];

void drawBarts()
{
    int length = sizeof(&barts) / sizeof(&barts[0]);
    for (int i = 0; i < length; i++)
    {
        if (barts[i].initialized)
            continue;
        C2D_DrawSprite(&barts[i].sprite);
    }
}

void initBarts(SpriteManager *spriteManager)
{
    int length = sizeof(&barts) / sizeof(&barts[0]);
    for (int i = 0; i < length; i++)
    {
        if (barts[i].initialized)
            continue;
        C2D_SpriteFromSheet(&barts[i].sprite, SpriteManager_GetSheet(spriteManager, "barts"), static_cast<int>(barts[i].type));
        C2D_SpriteSetPos(&barts[i].sprite, barts[i].x, barts[i].y);
    }
}

void addBart(float x, float y, BartType type)
{
    int length = sizeof(&barts) / sizeof(&barts[0]);
    for (int i = 0; i < length; i++)
    {
        if (!barts[i].initialized)
        {
            barts[i].x = x;
            barts[i].y = y;
            barts[i].type = type;
            barts[i].touched = false;
            barts[i].clicked = false;
            barts[i].initialized = true;
            return;
        }
    }
}