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

    // Start on frame 2
    C2D_SpriteFromSheet(&paints[0].sprite, copperSheet, 1);
    C2D_SpriteSetCenter(&paints[0].sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&paints[0].sprite, 160, 90); // center of top screen

    C2D_SpriteFromSheet(&paints[1].sprite, goldSheet, 1);
    C2D_SpriteSetCenter(&paints[1].sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&paints[1].sprite, 160, 90); // same center
}

void drawCopper()
{
    static int frame = 1;                // Start at frame 1 (not 0)
    static float frameTime = 0.1f;       // Seconds between frames

    // Update frame timer
    frameTime -= DeltaTime_Get();
    if (frameTime <= 0.0f) {
        frameTime = 0.05f;
        frame++;
        if (frame > 25)                  // Loop back to first frame
            frame = 1;
    } 

    // Get spritesheet from the manager
    C2D_SpriteSheet copperSheet = SpriteManager_GetSheet(&spriteManager, "Copper");

    // Load current animation frame image
    paints[0].sprite.image = C2D_SpriteSheetGetImage(copperSheet, frame);

    // Set sprite positions (required every frame)
    C2D_SpriteSetPos(&paints[0].sprite, paints[0].x, paints[0].y);

    // Draw the sprites
    C2D_DrawSprite(&paints[0].sprite);
}

void drawGold()
{
    static int frame = 1;                // Start at frame 1 (not 0)
    static float frameTime = 0.1f;       // Seconds between frames

    // Update frame timer
    frameTime -= DeltaTime_Get();
    if (frameTime <= 0.0f) {
        frameTime = 0.05f;
        frame++;
        if (frame > 25)                  // Loop back to first frame
            frame = 1;
    } 

    C2D_SpriteSheet goldSheet   = SpriteManager_GetSheet(&spriteManager, "Gold");

    // Load current animation frame image
    paints[1].sprite.image   = C2D_SpriteSheetGetImage(goldSheet, frame);

    C2D_SpriteSetPos(&paints[1].sprite,   paints[1].x, paints[1].y);

    C2D_DrawSprite(&paints[1].sprite);
}