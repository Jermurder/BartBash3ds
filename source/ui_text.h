// ui_text.h
#pragma once
#include <3ds.h>
#include <citro2d.h>

typedef struct {
    C2D_TextBuf textBuf;
    C2D_Text text;
    const char* content;

    float x, y;
    float scale;
    u32 color;
    C2D_Font font;
} UIText;

void UIText_Init(UIText* txt, const char* content, C2D_Font font, float x, float y, float scale, u32 color);
void UIText_SetText(UIText* txt, const char* newText);
void UIText_Draw(UIText* txt);
void UIText_Free(UIText* txt);
