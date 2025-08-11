#pragma once
#include <3ds.h>
#include <citro2d.h>

struct paintsprite  {
    int x, y;
    C2D_Sprite sprite;
};

extern paintsprite paints[2];
void initPaint();
void drawCopper();
void drawGold();