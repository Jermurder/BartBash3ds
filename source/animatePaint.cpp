#include "animatePaint.h"
#include "sprite_manager.h"

int frame;
float frameTime = 0.05f;

extern SpriteManager spriteManager;
paintsprite paints[2];

void initPaint()
{

    C2D_SpriteSheet copperSheet = SpriteManager_GetSheet(&spriteManager, "Copper");
    C2D_SpriteSheet goldSheet   = SpriteManager_GetSheet(&spriteManager, "Gold");

    C2D_SpriteFromSheet(&paints[0].sprite, copperSheet, 1);
    C2D_SpriteSetCenter(&paints[0].sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&paints[0].sprite, 160, 90);

    C2D_SpriteFromSheet(&paints[1].sprite, goldSheet, 1);
    C2D_SpriteSetCenter(&paints[1].sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&paints[1].sprite, 160, 90);
}

void drawCopper()
{
    static int frame = 1;
    static float frameTime = 0.1f;

    frameTime -= DeltaTime_Get();
    if (frameTime <= 0.0f) {
        frameTime = 0.05f;
        frame++;
        if (frame > 25)
            frame = 1;
    } 

    C2D_SpriteSheet copperSheet = SpriteManager_GetSheet(&spriteManager, "Copper");

    paints[0].sprite.image = C2D_SpriteSheetGetImage(copperSheet, frame);

    C2D_SpriteSetPos(&paints[0].sprite, paints[0].x, paints[0].y);

    C2D_DrawSprite(&paints[0].sprite);
}

void drawGold()
{
    static int frame = 1;
    static float frameTime = 0.1f;

    frameTime -= DeltaTime_Get();
    if (frameTime <= 0.0f) {
        frameTime = 0.05f;
        frame++;
        if (frame > 25)
            frame = 1;
    } 

    C2D_SpriteSheet goldSheet   = SpriteManager_GetSheet(&spriteManager, "Gold");

    paints[1].sprite.image   = C2D_SpriteSheetGetImage(goldSheet, frame);

    C2D_SpriteSetPos(&paints[1].sprite,   paints[1].x, paints[1].y);

    C2D_DrawSprite(&paints[1].sprite);
}