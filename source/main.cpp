#include "include.h"
#include <malloc.h>

constexpr int SCREEN_WIDTH = 400;
constexpr int SCREEN_HEIGHT = 240;

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000
static u32* SOC_buffer = nullptr;

static SpriteManager spriteManager;

int gamestate = 0; // 0 = menu, 1 = game

UIButton startButton, howtoplayButton;
C2D_Sprite mainmenuSprites[3];
touchPosition touch;

bool redrawTop = true;
bool redrawBottom = true;

SceneManager scenemanager;

// Store font globally for reuse
static C2D_Font font = nullptr;

void initSOC() {
    SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
if (!SOC_buffer) {
    // Can't use printf yet, fallback to crash
    svcBreak(USERBREAK_PANIC);
}

Result ret = socInit(SOC_buffer, SOC_BUFFERSIZE);
if (ret != 0) {
    svcBreak(USERBREAK_PANIC);
}

// redirect stdout/printf to 3dslink
link3dsStdio();
}

void texts() {
    font = C2D_FontLoad("romfs:/fonts/Helvetica.bcfnt");
    if (!font) {
        // Handle font loading failure here (log or fallback)
        return;
    }

    // Initialize labels directly on UIButton
    startButton.label = new UIText;
    startButton.label->Init("Start", font, startButton.x + 85, startButton.y + 50, 1.0f, C2D_Color32(0, 0, 0, 255));

    howtoplayButton.label = new UIText;
    howtoplayButton.label->Init("How to play", font, howtoplayButton.x + 43, howtoplayButton.y + 50, 1.0f, C2D_Color32(0, 0, 0, 255));
}

void drawTransition() {
    if (!scenemanager.isTransitioning) return;

    scenemanager.transitionProgress += DeltaTime_Get() * 1.0f;

    if (scenemanager.transitionPhase == TRANSITION_OUT) {
        float alpha = scenemanager.transitionProgress;
        if (alpha >= 1.0f) {
            alpha = 1.0f;
            scenemanager.currentScene = scenemanager.nextScene;
            scenemanager.transitionPhase = TRANSITION_IN;
            scenemanager.transitionProgress = 0.0f;
            redrawTop = true;
            redrawBottom = true;
        }
        C2D_DrawRectSolid(0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, C2D_Color32(0, 0, 0, static_cast<u8>(alpha * 255)));
    } else if (scenemanager.transitionPhase == TRANSITION_IN) {
        float alpha = 1.0f - scenemanager.transitionProgress;
        if (alpha <= 0.0f) {
            scenemanager.isTransitioning = false;
            scenemanager.transitionProgress = 0.0f;
            scenemanager.transitionPhase = TRANSITION_NONE;
        }
        C2D_DrawRectSolid(0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, C2D_Color32(0, 0, 0, static_cast<u8>(alpha * 255)));
    }
}

void onStartButtonClick() {
    changeScene(&scenemanager, 1);
    redrawTop = true;
    redrawBottom = true;
}

void loadUI() {
    UIButton_Init(&startButton, SpriteManager_GetSheet(&spriteManager, "UI2"), 2, (320 / 2) - 110, 30, 220, 80);
    UIButton_SetHoverSprite(&startButton, 1);
    UIButton_SetPressedSprite(&startButton, 0);

    UIButton_Init(&howtoplayButton, SpriteManager_GetSheet(&spriteManager, "UI2"), 2, (320 / 2) - 110, 130, 220, 80);
    UIButton_SetHoverSprite(&howtoplayButton, 1);
    UIButton_SetPressedSprite(&howtoplayButton, 0);
}

void loadSprites() {
    C2D_SpriteFromSheet(&mainmenuSprites[0], SpriteManager_GetSheet(&spriteManager, "UI1"), 1);
    C2D_SpriteSetPos(&mainmenuSprites[0], 0, 0);

    C2D_SpriteFromSheet(&mainmenuSprites[1], SpriteManager_GetSheet(&spriteManager, "logo"), 0);
    C2D_SpriteSetPos(&mainmenuSprites[1], 45, 50);
    C2D_SpriteSetScale(&mainmenuSprites[1], 0.5f, 0.5f);

    C2D_SpriteFromSheet(&mainmenuSprites[2], SpriteManager_GetSheet(&spriteManager, "UI3"), 0);
}

void drawTop(C3D_RenderTarget* target) {
    C2D_TargetClear(target, C2D_Color32f(0, 0, 0, 1));
    C2D_SceneBegin(target);

    if (scenemanager.currentScene == 0) {
        C2D_DrawSprite(&mainmenuSprites[0]);
        C2D_DrawSprite(&mainmenuSprites[1]);

    } else if (scenemanager.currentScene == 1) {
        C2D_Sprite background;
        C2D_SpriteFromSheet(&background, SpriteManager_GetSheet(&spriteManager, "UI3"), 1);
        C2D_DrawSprite(&background);
    }
}

void drawBottom(C3D_RenderTarget* target) {
    hidTouchRead(&touch);
    C2D_TargetClear(target, C2D_Color32(0, 0, 0, 0));
    C2D_SceneBegin(target);

    if (scenemanager.currentScene == 0) {
        C2D_DrawSprite(&mainmenuSprites[2]);
        UIButton_Update(&startButton, touch);
        UIButton_Draw(&startButton);
        UIButton_Update(&howtoplayButton, touch); 
        UIButton_Draw(&howtoplayButton);
    }
}


int main(int argc, char* argv[]) {
    initSOC();
    romfsInit();
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    auto* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    auto* bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    SpriteManager_Init(&spriteManager);
    SpriteManager_Load(&spriteManager, "UI1", "romfs:/gfx/UI1.t3x");
    SpriteManager_Load(&spriteManager, "UI2", "romfs:/gfx/UI2.t3x");
    SpriteManager_Load(&spriteManager, "UI3", "romfs:/gfx/backgrounds.t3x");
    SpriteManager_Load(&spriteManager, "logo", "romfs:/gfx/logo.t3x"); 

    loadUI();
    texts();
    loadSprites();

    while (aptMainLoop()) {
    	DeltaTime_Update();
        float dt = DeltaTime_Get();


        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break;

        startButton.onClick = onStartButtonClick;
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
     	drawTop(top);
        drawTransition();
       	drawBottom(bottom);
        drawTransition();

        C3D_FrameEnd(0);
    }

    SpriteManager_Free(&spriteManager);

    if (startButton.label) {
        startButton.label->Free();
        delete startButton.label;
        startButton.label = nullptr;
    }
    if (howtoplayButton.label) {
        howtoplayButton.label->Free();
        delete howtoplayButton.label;
        howtoplayButton.label = nullptr;
    }

    if (font) {
        C2D_FontFree(font);
        font = nullptr;
    }

    C2D_Fini();
    C3D_Fini();
    gfxExit();
    romfsExit();
    socExit();
    return 0;
}
