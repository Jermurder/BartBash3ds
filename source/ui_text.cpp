// ui_text.cpp
#include "ui_text.h"

void UIText::Init(const std::string &txtContent, C2D_Font f, float posX, float posY, float s, u32 col)
{
    content = txtContent;
    font = f;
    x = posX;
    y = posY;
    scale = s;
    color = col;

    if (textBuf)
    {
        C2D_TextBufDelete(textBuf);
        textBuf = nullptr;
    }

    textBuf = C2D_TextBufNew(content.size() + 1);
    C2D_TextParse(&text, textBuf, content.c_str());
    C2D_TextOptimize(&text);
}

void UIText::SetText(const std::string &newText)
{
    content = newText;

    if (textBuf)
    {
        C2D_TextBufDelete(textBuf);
        textBuf = nullptr;
    }

    textBuf = C2D_TextBufNew(content.size() + 1);
    C2D_TextParse(&text, textBuf, content.c_str());
    C2D_TextOptimize(&text);
}

void UIText::Draw() const
{
    C2D_DrawText(&text, C2D_AtBaseline | C2D_WithColor, x, y, 0.0f, scale, scale, color);
}

void UIText::Free()
{
    if (textBuf)
    {
        C2D_TextBufDelete(textBuf);
        textBuf = nullptr;
    }
}
