#include "include.h"
#include <malloc.h>

constexpr int SCREEN_WIDTH = 400;
constexpr int SCREEN_HEIGHT = 240;

#define SOC_ALIGN 0x1000
#define SOC_BUFFERSIZE 0x100000
static u32 *SOC_buffer = nullptr;

static SpriteManager spriteManager;

int gamestate = 0; // 0 = menu, 1 = game
int selectedBarts;

int bartphase = 0; // Select, Drop, Dropped
int multiplier = 1;
int score;
int totalScore;

UIText scoreText;
UIText selectedText;
UIButton startButton, howtoplayButton;
C2D_Sprite mainmenuSprites[3];

touchPosition touch;
u32 kDown;
u16 touchX;
u32 kHeld;

UIText Multiplier;

bool redrawTop = true;
bool redrawBottom = true;

SceneManager scenemanager;

bool startcounting;

// Store font globally for reuse
static C2D_Font font = nullptr;

void initSOC()
{
    SOC_buffer = (u32 *)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
    if (!SOC_buffer)
    {
        // Can't use printf yet, fallback to crash
        svcBreak(USERBREAK_PANIC);
    }

    Result ret = socInit(SOC_buffer, SOC_BUFFERSIZE);
    if (ret != 0)
    {
        svcBreak(USERBREAK_PANIC);
    }

    // redirect stdout/printf to 3dslink
    link3dsStdio();
}

void texts()
{
    font = C2D_FontLoad("romfs:/fonts/Helvetica.bcfnt");
    if (!font)
    {
        // Handle font loading failure here (log or fallback)
        return;
    }

    // Initialize labels directly on UIButton
    startButton.label = new UIText;
    startButton.label->Init("Start", font, startButton.x + 85, startButton.y + 50, 1.0f, C2D_Color32(0, 0, 0, 255));

    howtoplayButton.label = new UIText;
    howtoplayButton.label->Init("How to play", font, howtoplayButton.x + 43, howtoplayButton.y + 50, 1.0f, C2D_Color32(0, 0, 0, 255));
    Multiplier.Init("Multiplier: " + std::to_string(multiplier) + "x", font, 157, 142, 0.5f, C2D_Color32(255, 255, 255, 255));
    scoreText.Init("Score: " + std::to_string(score), font, 30, 142, 0.5f, C2D_Color32(255, 255, 255, 255));
    selectedText.Init("Selected: " + std::to_string(selectedBarts) + "/6", font, 295, 142, 0.5f, C2D_Color32(255, 255, 255, 255));
}

void drawTransition()
{
    if (!scenemanager.isTransitioning)
        return;

    scenemanager.transitionProgress += DeltaTime_Get() * 1.0f;

    if (scenemanager.transitionPhase == TRANSITION_OUT)
    {
        float alpha = scenemanager.transitionProgress;
        if (alpha >= 1.0f)
        {
            alpha = 1.0f;
            scenemanager.currentScene = scenemanager.nextScene;
            scenemanager.transitionPhase = TRANSITION_IN;
            scenemanager.transitionProgress = 0.0f;
            redrawTop = true;
            redrawBottom = true;
        }
        C2D_DrawRectSolid(0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, C2D_Color32(0, 0, 0, static_cast<u8>(alpha * 255)));
    }
    else if (scenemanager.transitionPhase == TRANSITION_IN)
    {
        float alpha = 1.0f - scenemanager.transitionProgress;
        if (alpha <= 0.0f)
        {
            scenemanager.isTransitioning = false;
            scenemanager.transitionProgress = 0.0f;
            scenemanager.transitionPhase = TRANSITION_NONE;
        }
        C2D_DrawRectSolid(0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, C2D_Color32(0, 0, 0, static_cast<u8>(alpha * 255)));
    }
}

void onStartButtonClick()
{
    changeScene(&scenemanager, 1);
    audioManagerStop();
    audioManagerPlay("romfs:/sounds/bashs.opus");
    redrawTop = true;
    redrawBottom = true;
}

void loadUI()
{
    UIButton_Init(&startButton, SpriteManager_GetSheet(&spriteManager, "UI2"), 2, (320 / 2) - 110, 30, 220, 80);
    UIButton_SetHoverSprite(&startButton, 1);
    UIButton_SetPressedSprite(&startButton, 0);

    UIButton_Init(&howtoplayButton, SpriteManager_GetSheet(&spriteManager, "UI2"), 2, (320 / 2) - 110, 130, 220, 80);
    UIButton_SetHoverSprite(&howtoplayButton, 1);
    UIButton_SetPressedSprite(&howtoplayButton, 0);
}

void loadSprites()
{
    C2D_SpriteFromSheet(&mainmenuSprites[0], SpriteManager_GetSheet(&spriteManager, "UI1"), 1);
    C2D_SpriteSetPos(&mainmenuSprites[0], 0, 0);

    C2D_SpriteFromSheet(&mainmenuSprites[1], SpriteManager_GetSheet(&spriteManager, "logo"), 0);
    C2D_SpriteSetPos(&mainmenuSprites[1], 45, 50);
    C2D_SpriteSetScale(&mainmenuSprites[1], 0.5f, 0.5f);

    C2D_SpriteFromSheet(&mainmenuSprites[2], SpriteManager_GetSheet(&spriteManager, "UI3"), 0);
}

void drawTop(C3D_RenderTarget *target)
{
    C2D_TargetClear(target, C2D_Color32f(0, 0, 0, 1));
    C2D_SceneBegin(target);

    if (scenemanager.currentScene == 0)
    {
        C2D_DrawSprite(&mainmenuSprites[0]);
        C2D_DrawSprite(&mainmenuSprites[1]);
    }
    else if (scenemanager.currentScene == 1)
    {
        C2D_Sprite display;
        C2D_SpriteFromSheet(&display, SpriteManager_GetSheet(&spriteManager, "UI4"), 0);

        C2D_SpriteSetPos(&display, 13, 100);
        C2D_DrawSprite(&display);
        scoreText.SetText("Score: " + std::to_string(score));
        scoreText.Draw();

        C2D_SpriteSetPos(&display, 146, 100);
        C2D_DrawSprite(&display);
        Multiplier.SetText("Multiplier: " + std::to_string(multiplier) + "x");
        Multiplier.Draw();

        C2D_SpriteSetPos(&display, 279, 100);
        C2D_DrawSprite(&display);
        selectedText.SetText("Selected: " + std::to_string(selectedBarts) + "/6");
        selectedText.Draw();
    }
}

void drawBottom(C3D_RenderTarget *target)
{
    C2D_TargetClear(target, C2D_Color32(0, 0, 0, 0));
    C2D_SceneBegin(target);

    if (scenemanager.currentScene == 0)
    {
        C2D_DrawSprite(&mainmenuSprites[2]);
        UIButton_Update(&startButton, touch);
        UIButton_Draw(&startButton);
        UIButton_Update(&howtoplayButton, touch);
        UIButton_Draw(&howtoplayButton);
    }
    else if (scenemanager.currentScene == 1)
    {
        C2D_Sprite background;
        C2D_SpriteFromSheet(&background, SpriteManager_GetSheet(&spriteManager, "UI3"), 1);
        C2D_SpriteSetPos(&background, 28, 0);
        C2D_DrawSprite(&background);
        drawBarts();
        b2Body *player = PhysicsManager_GetPlayer();
        if (player)
        {
            b2Vec2 pos = player->GetPosition();
            float px = MetersToPixels(pos.x);
            float py = MetersToPixels(pos.y);
            C2D_Sprite playerSprite;

            C2D_SpriteFromSheet(&playerSprite, SpriteManager_GetSheet(&spriteManager, "barts"), 0);
            C2D_SpriteSetPos(&playerSprite, px, py);
            C2D_SpriteSetRotation(&playerSprite, player->GetAngle());
            C2D_SpriteSetCenter(&playerSprite, 0.5f, 0.5f);
            C2D_DrawSprite(&playerSprite);
        }
        if (bartphase == 0)
        {
            player->SetType(b2_staticBody);
            if (kDown & KEY_TOUCH)
            {
                findBart(touch, &selectedBarts, &spriteManager);
            }
            if (kDown & KEY_A && selectedBarts > 0)
            {
                bartphase = 1;
                PhysicsManager_SpawnPlayer(190, 50);

                deinitBart(firstBart);
            }
        }
        else if (bartphase == 1)
        {
            player->SetType(b2_staticBody);
            if (kHeld & KEY_TOUCH && touch.px > 30 && touch.px < 290)
            {
                touchX = touch.px;
            }
            player->SetTransform(b2Vec2(PixelsToMeters(touchX), PixelsToMeters(50)), 0);

            if (kDown & KEY_A)
            {
                player->SetType(b2_dynamicBody);
                bartphase = 2;
            }
        }
    }
}

void updateBarts(float deltaTime, SpriteManager* spriteManager) {
    for (int i = 0; i < 40; ++i) {
        if (!barts[i].initialized) continue;
        updateBartFading(&barts[i], spriteManager, deltaTime);
    }
}

int main(int argc, char *argv[])
{
    initSOC();
    romfsInit();
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    auto *top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    auto *bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    SpriteManager_Init(&spriteManager);
    SpriteManager_Load(&spriteManager, "UI1", "romfs:/gfx/UI1.t3x");
    SpriteManager_Load(&spriteManager, "UI2", "romfs:/gfx/UI2.t3x");
    SpriteManager_Load(&spriteManager, "UI3", "romfs:/gfx/backgrounds.t3x");
    SpriteManager_Load(&spriteManager, "logo", "romfs:/gfx/logo.t3x");
    SpriteManager_Load(&spriteManager, "barts", "romfs:/gfx/barts.t3x");
    SpriteManager_Load(&spriteManager, "UI4", "romfs:/gfx/UI4.t3x");

    loadUI();
    texts();
    loadSprites();
    audioManagerInit();
    audioManagerPlay("romfs:/sounds/bort.opus");

    srand(static_cast<unsigned int>(time(NULL)));
    PhysicsManager_Init();
    spawnBarts();
    initBarts(&spriteManager);

    while (aptMainLoop())
    {
        DeltaTime_Update();
        float dt = DeltaTime_Get();
        hidScanInput();
        kDown = hidKeysDown();
        kHeld = hidKeysHeld();
        if (kDown & KEY_START)
            break;
        hidTouchRead(&touch);
        PhysicsManager_Update(1.0f / 60.0f);
        updateBartsAfterPhysics();
        startButton.onClick = onStartButtonClick;
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        drawTop(top);
        drawTransition();
        drawBottom(bottom);
        drawTransition();
        counting();
        C3D_FrameEnd(0);
    }

    SpriteManager_Free(&spriteManager);

    if (startButton.label)
    {
        startButton.label->Free();
        delete startButton.label;
        startButton.label = nullptr;
    }
    if (howtoplayButton.label)
    {
        howtoplayButton.label->Free();
        delete howtoplayButton.label;
        howtoplayButton.label = nullptr;
    }

    if (font)
    {
        C2D_FontFree(font);
        font = nullptr;
    }
    audioManagerExit();
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    romfsExit();
    socExit();
    return 0;
}
