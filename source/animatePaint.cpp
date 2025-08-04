#include "animatePaint.h"
#include "sprite_manager.h"

int frame;
float frameTime = 0.05f;
C2D_Sprite copperSprite;
C2D_Sprite goldSprite;

extern SpriteManager spriteManager;


void initPaint()
{

    C2D_SpriteSheet copperSheet = SpriteManager_GetSheet(&spriteManager, "Copper");
    C2D_SpriteSheet goldSheet   = SpriteManager_GetSheet(&spriteManager, "Gold");

    // Start on frame 2
    C2D_SpriteFromSheet(&copperSprite, copperSheet, 1);
    C2D_SpriteSetCenter(&copperSprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&copperSprite, 160, 90); // center of top screen

    C2D_SpriteFromSheet(&goldSprite, goldSheet, 1);
    C2D_SpriteSetCenter(&goldSprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&goldSprite, 160, 90); // same center
}

void spinPaint()
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
    C2D_SpriteSheet goldSheet   = SpriteManager_GetSheet(&spriteManager, "Gold");

    // Load current animation frame image
    copperSprite.image = C2D_SpriteSheetGetImage(copperSheet, frame);
    goldSprite.image   = C2D_SpriteSheetGetImage(goldSheet, frame);

    // Set sprite positions (required every frame)
    C2D_SpriteSetPos(&copperSprite, 100.0f, 100.0f);
    C2D_SpriteSetPos(&goldSprite,   200.0f, 100.0f);

    // Draw the sprites
    C2D_DrawSprite(&copperSprite);
    C2D_DrawSprite(&goldSprite);
}