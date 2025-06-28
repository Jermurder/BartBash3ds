
#include <stdlib.h>
#include "ui_text.h"

void UIText_Init(UIText* txt, const char* content, C2D_Font font, float x, float y, float scale, u32 color) {
    txt->font = font;
    txt->x = x;
    txt->y = y;
    txt->scale = scale;
    txt->color = color;
    txt->content = strdup(content);

    txt->textBuf = C2D_TextBufNew(strlen(content) + 1);
    C2D_TextParse(&txt->text, txt->textBuf, txt->content);
    C2D_TextOptimize(&txt->text);
}

void UIText_SetText(UIText* txt, const char* newText) {
    if (txt->content) free((void*)txt->content);
    txt->content = strdup(newText);

    C2D_TextBufClear(txt->textBuf);
    C2D_TextParse(&txt->text, txt->textBuf, txt->content);
    C2D_TextOptimize(&txt->text);
}

void UIText_Draw(UIText* txt) {
    
    C2D_DrawText(&txt->text, C2D_AtBaseline | C2D_WithColor, txt->x, txt->y, 0.0f, txt->scale, txt->scale, txt->color);
}

void UIText_Free(UIText* txt) {
    C2D_TextBufDelete(txt->textBuf);
    free((void*)txt->content);
}
