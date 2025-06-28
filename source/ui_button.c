#include "ui_button.h"

void UIButton_Init(UIButton* btn, C2D_SpriteSheet sheet, int spriteNormal, float x, float y, float width, float height) {
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
}

void UIButton_SetHoverSprite(UIButton* btn, int spriteHover) {
    btn->spriteHover = spriteHover;
    btn->hasHover = true;
}

void UIButton_SetPressedSprite(UIButton* btn, int spritePressed) {
    btn->spritePressed = spritePressed;
    btn->hasPressed = true;
}

void UIButton_Update(UIButton* btn, touchPosition touch) {
    btn->hovered = touch.px >= btn->x && touch.px <= btn->x + btn->width &&
                   touch.py >= btn->y && touch.py <= btn->y + btn->height;

    u32 kDown = hidKeysDown();
    if (btn->hovered && (kDown & KEY_TOUCH)) {
        btn->pressed = true;
    }

    u32 kUp = hidKeysUp();
    if (btn->pressed && (kUp & KEY_TOUCH)) {
        btn->onClick();
        btn->pressed = false;
    }
}

void UIButton_Draw(UIButton* btn) {
    int spriteIndex = btn->spriteNormal;

    if (btn->pressed && btn->hasPressed) {
        spriteIndex = btn->spritePressed;
    } else if (btn->hovered && btn->hasHover) {
        spriteIndex = btn->spriteHover;
    }

    C2D_Sprite sprite;
    C2D_SpriteFromSheet(&sprite, btn->sheet, spriteIndex);
    C2D_SpriteSetPos(&sprite, btn->x, btn->y);
    C2D_SpriteSetScale(&sprite, 1, 1);
    C2D_DrawSprite(&sprite);

    // Draw label if attached
    if (btn->label) {
        UIText_Draw(btn->label);
    }
}