#pragma once

#include <string>
#include <3ds.h>
#include <citro2d.h>

struct UIText
{
    std::string content;
    C2D_Font font;
    float x, y;
    float scale;
    u32 color;

    C2D_TextBuf textBuf = nullptr;
    C2D_Text text;

    void Init(const std::string &txtContent, C2D_Font f, float posX, float posY, float s, u32 col);

    void SetText(const std::string &newText);

    void Draw() const;

    void Free();
};
