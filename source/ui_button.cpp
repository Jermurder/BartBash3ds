#include "ui_button.h"
#include <3ds.h>     // For hidKeysDown, hidKeysUp and touchPosition
#include <citro2d.h> // For C2D_Sprite

void UIButton_Init(UIButton *btn, C2D_SpriteSheet sheet, int spriteNormal, float x, float y, float width, float height, u32 color, bool outline, bool sound)
{
    btn->sheet = sheet;
    btn->spriteNormal = spriteNormal;
    btn->spritePressed = spriteNormal;
    btn->x = x;
    btn->y = y;
    btn->width = width;
    btn->height = height;
    btn->onClick = NULL;
    btn->pressed = false;
    btn->hovered = false;
    btn->hasHover = false;
    btn->hasPressed = false;
    btn->label = NULL;
    btn->spriteHover = 0;
    btn->color = color;
    btn->outline = outline;
    btn->sound = sound;
}

void UIButton_SetHoverSprite(UIButton *btn, int spriteHover)
{
    btn->spriteHover = spriteHover;
    btn->hasHover = true;
}

void UIButton_SetPressedSprite(UIButton *btn, int spritePressed)
{
    btn->spritePressed = spritePressed;
    btn->hasPressed = true;
}

void UIButton_Update(UIButton *btn, touchPosition touch)
{
    // Check if touch is within button bounds
    btn->hovered = (touch.px >= btn->x && touch.px <= btn->x + btn->width &&
                    touch.py >= btn->y && touch.py <= btn->y + btn->height);

    u32 kDown = hidKeysDown();
    u32 kUp = hidKeysUp();

    if (btn->hovered && (kDown & KEY_TOUCH))
    {
        btn->pressed = true;
    }

    if (btn->pressed && (kUp & KEY_TOUCH))
    {
        btn->pressed = false;
        if (btn->onClick)
        {
            btn->onClick();
            if (btn->sound)
            {
                AudioManager::Play("romfs:/sounds/key.opus", 1.0f, false, 1.0f, 0.0f);
            }
        }
    }
}

void UIButton_Draw(UIButton *btn)
{
    uint8_t byte0 = (uint8_t)btn->color;
    if (btn->spriteNormal == -1)
    {
        if (btn->toggled && btn->outline)
        {
            C2D_DrawRectSolid(btn->x - 2, btn->y -2, 0.0f, btn->width + 4, btn->height + 4, C2D_Color32(255, 255, 255, 255));
        }
        C2D_DrawRectSolid(btn->x, btn->y, 0, btn->width, btn->height, btn->color);
        if (btn->label)
        {
            btn->label->Draw();
        }
        return;
    }
    int spriteIndex = btn->spriteNormal;

    if (btn->pressed && btn->hasPressed)
    {
        spriteIndex = btn->spritePressed;
    }
    else if (btn->hovered && btn->hasHover)
    {
        spriteIndex = btn->spriteHover;
    }

    C2D_Sprite sprite;
    C2D_SpriteFromSheet(&sprite, btn->sheet, spriteIndex);
    C2D_SpriteSetPos(&sprite, btn->x, btn->y);
    C2D_SpriteSetScale(&sprite, 1, 1);
    C2D_DrawSprite(&sprite);

    // Draw label if attached
    if (btn->label)
    {
        btn->label->Draw();
    }
}
