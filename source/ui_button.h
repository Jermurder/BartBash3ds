#pragma once
#include <3ds.h>
#include <citro2d.h>
#include "ui_text.h" // Required because you use UIText*
#include "audio_manager.h" // Required for audio functions
typedef struct
{
    int x, y, width, height;
    C2D_SpriteSheet sheet;
    int spriteNormal, spriteHover, spritePressed;
    bool hasHover, hasPressed;
    u32 color;
    bool toggled;
    bool outline;
    bool sound;

    bool hovered, pressed;
    void (*onClick)(void);

    UIText *label; // optional text label
} UIButton;

void UIButton_Init(UIButton *btn, C2D_SpriteSheet sheet, int spriteNormal, float x, float y, float width, float height, u32 color, bool outline, bool sound);
void UIButton_SetHoverSprite(UIButton *btn, int spriteHover);
void UIButton_SetPressedSprite(UIButton *btn, int spritePressed);
void UIButton_Update(UIButton *btn, touchPosition touch);
void UIButton_Draw(UIButton *btn);

