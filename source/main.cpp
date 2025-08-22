#include "include.h"
#include <malloc.h>

constexpr int SCREEN_WIDTH = 400;
constexpr int SCREEN_HEIGHT = 240;
constexpr int BOTTOMSCREEN_WIDTH = 320;
constexpr int BOTTOMSCREEN_HEIGHT = 240;

#define SOC_ALIGN 0x1000
#define SOC_BUFFERSIZE 0x100000
static u32 *SOC_buffer = nullptr;

// Persistent state
bool holdingCopper = false;
bool holdingGold = false;

int releaseX = 0;
int releaseY = 0;

bool fading = false;
bool fadeOut = true; // true = fading out, false = fading in
float backgroundOpacity = 0.0f;
int nextPhase = 0; // new phase to switch to after fade out

SpriteManager spriteManager;

int gamestate = 0; // 0 = menu, 1 = game
int selectedBarts;
int storephase; // 0 menu 1 copper 2 gold

int bartphase = 0; // Select, Drop, Dropped
int multiplier = 1;
int score;
int totalScore;
int currentRound;
int *currentRoundPtr = &currentRound;
int gems;

bool playerEnabled = false;
bool *playerEnabledPtr = &playerEnabled;

b2Body *player;

UIText scoreText;
UIText selectedText;
UIText howtoplayText;
UIText endScore;
UIText endGems;
UIText storeGems;
UIButton startButton, howtoplayButton, itemsButton, goldPaint, copperPaint;
C2D_Sprite mainmenuSprites[3];
UIText copperBartText, goldBartText;

touchPosition touch;
u32 kDown;
u16 touchX = 200;
u32 kHeld;
u32 kUp;

int copperPaintCount = 1;
int goldPaintCount = 1;

UIText copperamount;
UIText goldamount;

UIText Multiplier;
UIText tutorial;

UIButton continuebutton, quitbutton, storebutton;

UIButton backToEnd;

UIButton backButton, buyButton;

bool redrawTop = true;
bool redrawBottom = true;

SceneManager scenemanager;

bool startcounting;

// Store font globally for reuse
static C2D_Font font = nullptr;

// Add a global variable to store the current audio handle
AudioManager::AudioHandle currentAudioHandle = 0;

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

void continuegame()
{
    changeScene(&scenemanager, 1);
    if (currentAudioHandle) {
        AudioManager::StopAudio(currentAudioHandle);
    }
    currentAudioHandle = AudioManager::Play("romfs:/sounds/bashs.opus", 1.0f, false, 1.0f, 0.0f);
    resetBarts();
    score = 0;
    totalScore = 0;
    multiplier = 1;
    currentRound = 0;
    startcounting = false;
    selectedBarts = 0;
}

void quitgame()
{
    changeScene(&scenemanager, 0);
    score = 0;
    totalScore = 0;
    multiplier = 1;
    currentRound = 0;
    startcounting = false;
    selectedBarts = 0;
}

void store()
{
    changeScene(&scenemanager, 5);
}
void backButtonPress()
{
    storephase = 0;
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
    Multiplier.Init("Multiplier: " + std::to_string(multiplier) + "x", font, 150, 142, 0.5f, C2D_Color32(255, 255, 255, 255));
    scoreText.Init("Score: " + std::to_string(score), font, 23, 142, 0.5f, C2D_Color32(255, 255, 255, 255));
    selectedText.Init("Selected: " + std::to_string(selectedBarts) + "/6", font, 288, 142, 0.5f, C2D_Color32(255, 255, 255, 255));

    howtoplayText.Init("A to exit", font, 60, 110, 2.0f, C2D_Color32(255, 255, 255, 255));
    endScore.Init("Score: \n" + std::to_string(totalScore), font, 100, 130, 1.0f, C2D_Color32(0, 0, 255, 255));
    endGems.Init("Gems: \n" + std::to_string(gems), font, 210, 130, 1.0f, C2D_Color32(0, 0, 255, 255));
    itemsButton.label = new UIText;
    itemsButton.label->Init("Items", font, itemsButton.x + 8, itemsButton.y + 18, 0.5f, C2D_Color32(255, 255, 255, 255));

    copperamount.Init("(" + std::to_string(copperPaintCount) + ")", font, 40 + 50, 125 - 45, 0.5f, C2D_Color32(255, 255, 255, 255));
    goldamount.Init("(" + std::to_string(goldPaintCount) + ")", font, 110 + 45, 125 - 45, 0.5f, C2D_Color32(255, 255, 255, 255));

    continuebutton.label = new UIText;
    continuebutton.label->Init("Continue", font, continuebutton.x + 55, continuebutton.y + 50, 1.0f, C2D_Color32(0, 0, 0, 255));
    storebutton.label = new UIText;
    storebutton.label->Init("Store", font, storebutton.x + 60, storebutton.y + 50, 1.0f, C2D_Color32(0, 0, 0, 255));
    storeGems.Init("BartGems: " + std::to_string(gems), font, SCREEN_WIDTH / 2 - 40, SCREEN_HEIGHT / 2, 0.5f, C2D_Color32(255, 255, 255, 255));
    copperBartText.Init("Copper BartPaint", font, 30, 30, 1.0f, C2D_Color32(255, 255, 255, 255));
    goldBartText.Init("Gold BartPaint", font, 30, 30, 1.0f, C2D_Color32(255, 255, 255, 255));

    buyButton.label = new UIText;
    buyButton.label->Init("Buy", font, buyButton.x + 93, buyButton.y + 50, 1.0f, C2D_Color32(0, 0, 0, 255));
    backButton.label = new UIText;
    backButton.label->Init("Back", font, backButton.x + 87, backButton.y + 50, 1.0f, C2D_Color32(0, 0, 0, 255));

    backToEnd.label = new UIText;
    backToEnd.label->Init("Back", font, backToEnd.x + 12, backToEnd.y + 17, 0.5f, C2D_Color32(255, 255, 255, 255));
    
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
    if (currentAudioHandle) {
        AudioManager::StopAudio(currentAudioHandle);
    }
    currentAudioHandle = AudioManager::Play("romfs:/sounds/bashs.opus", 1.0f, false, 1.0f, 0.0f);
    redrawTop = true;
    redrawBottom = true;
}

void onHowToPlayButtonClick()
{
    changeScene(&scenemanager, 2);
}

void onItemsButtonClick()
{
    itemsButton.toggled = !itemsButton.toggled;
}

void onGoldPaintButtonClick()
{
    goldPaint.toggled = !goldPaint.toggled;
    if (copperPaint.toggled)
    {
        copperPaint.toggled = false; // Disable copper paint if gold is selected
    }
}

void onCopperPaintButtonClick()
{
    copperPaint.toggled = !copperPaint.toggled;
    if (goldPaint.toggled)
    {
        goldPaint.toggled = false; // Disable gold paint if copper is selected
    }
}

void buyPaint()
{
    if (storephase == 1)
    {
        if (gems >= 5)
        {
            gems -= 5;
            copperPaintCount++;
            copperamount.SetText("(" + std::to_string(copperPaintCount) + ")");
            AudioManager::Play("romfs:/sounds/cash.opus", 1.0f, false, 1.0f, 0.0f);
        }
    }
    else if (storephase == 2)
    {
        if (gems >= 10)
        {
            gems -= 10;
            goldPaintCount++;
            goldamount.SetText("(" + std::to_string(goldPaintCount) + ")");
            AudioManager::Play("romfs:/sounds/cash.opus", 1.0f, false, 1.0f, 0.0f);
        }
    }
}

void goToEnd()
{
    changeScene(&scenemanager, 4);
}

void loadUI()
{
    UIButton_Init(&startButton, SpriteManager_GetSheet(&spriteManager, "UI2"), 2, (320 / 2) - 110, 30, 220, 80, NULL, false, true);
    UIButton_SetHoverSprite(&startButton, 1);
    UIButton_SetPressedSprite(&startButton, 0);

    UIButton_Init(&howtoplayButton, SpriteManager_GetSheet(&spriteManager, "UI2"), 2, (320 / 2) - 110, 130, 220, 80, NULL, false, true);
    UIButton_SetHoverSprite(&howtoplayButton, 1);
    UIButton_SetPressedSprite(&howtoplayButton, 0);



    UIButton_Init(&itemsButton, NULL, -1, 10, 5, 50, 30, C2D_Color32(255, 0, 0, 255), true, false);

    UIButton_Init(&copperPaint, NULL, -1, 20 + 50, 5, 60, 80, C2D_Color32(0, 0, 0, 255), true, false);

    UIButton_Init(&goldPaint, NULL, -1, 85 + 50, 5, 60, 80, C2D_Color32(0, 0, 0, 255), true, false);

    UIButton_Init(&continuebutton, SpriteManager_GetSheet(&spriteManager, "UI2"), 2, (320 / 2) - 110, 30, 220, 80, NULL, false, true);
    UIButton_SetHoverSprite(&continuebutton, 1);
    UIButton_SetPressedSprite(&continuebutton, 0);

    UIButton_Init(&storebutton, SpriteManager_GetSheet(&spriteManager, "UI2"), 2, (320 / 2) - 110, 130, 220, 80, NULL, false, true);
    UIButton_SetHoverSprite(&storebutton, 1);
    UIButton_SetPressedSprite(&storebutton, 0);

    UIButton_Init(&buyButton, SpriteManager_GetSheet(&spriteManager, "UI2"), 2, (320 / 2) - 110, 30, 220, 80, NULL, false, true);
    UIButton_SetHoverSprite(&buyButton, 1);
    UIButton_SetPressedSprite(&buyButton, 0);

    UIButton_Init(&backButton, SpriteManager_GetSheet(&spriteManager, "UI2"), 2, (320 / 2) - 110, 130, 220, 80, NULL, false, true);
    UIButton_SetHoverSprite(&backButton, 1);
    UIButton_SetPressedSprite(&backButton, 0);

    UIButton_Init(&backToEnd, NULL, -1, 10, 10, 60, 30, C2D_Color32(0, 0, 0, 200), false, true);

    startButton.onClick = onStartButtonClick;
    howtoplayButton.onClick = onHowToPlayButtonClick;
    itemsButton.onClick = onItemsButtonClick;
    copperPaint.onClick = onCopperPaintButtonClick;
    goldPaint.onClick = onGoldPaintButtonClick;
    continuebutton.onClick = continuegame;
    quitbutton.onClick = quitgame;
    storebutton.onClick = store;
    backButton.onClick = backButtonPress;
    buyButton.onClick = buyPaint;
    backToEnd.onClick = goToEnd;
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
        C2D_DrawSprite(&mainmenuSprites[2]);
        C2D_Sprite display;
        C2D_SpriteFromSheet(&display, SpriteManager_GetSheet(&spriteManager, "UI4"), 0);

        C2D_SpriteSetPos(&display, 7, 100);
        C2D_DrawSprite(&display);
        scoreText.SetText("Score: " + std::to_string(totalScore));
        scoreText.Draw();

        C2D_SpriteSetPos(&display, 139, 100);
        C2D_DrawSprite(&display);
        Multiplier.SetText("Multiplier: " + std::to_string(multiplier) + "x");
        Multiplier.Draw();

        C2D_SpriteSetPos(&display, 272, 100);
        C2D_DrawSprite(&display);
        selectedText.SetText("Selected: " + std::to_string(selectedBarts) + "/6");
        selectedText.Draw();
    }
    else if (scenemanager.currentScene == 2)
    {
        C2D_Sprite background;
        C2D_SpriteFromSheet(&background, SpriteManager_GetSheet(&spriteManager, "UI3"), 2);
        C2D_SpriteSetPos(&background, 0, 0);
        C2D_DrawSprite(&background);
        if (kDown & KEY_A)
        {
            changeScene(&scenemanager, 0);
        }
    }

    else if (scenemanager.currentScene == 4)
    {
        C2D_Sprite background;
        C2D_SpriteFromSheet(&background, SpriteManager_GetSheet(&spriteManager, "UI1"), 0);
        C2D_SpriteSetPos(&background, 0, 0);
        C2D_DrawSprite(&background);
        C2D_SpriteFromSheet(&background, SpriteManager_GetSheet(&spriteManager, "UI2"), 3);
        C2D_SpriteSetPos(&background, 84, 37);
        C2D_DrawSprite(&background);
        endScore.SetText("Score: \n" + std::to_string(totalScore));
        endScore.Draw();
        endGems.SetText("Gems: \n" + std::to_string(gems));
        endGems.Draw();
    }
    else if (scenemanager.currentScene == 5)
    {
        if (storephase == 0)
        {
            C2D_Sprite display;
            C2D_SpriteFromSheet(&display, SpriteManager_GetSheet(&spriteManager, "UI4"), 0);
            C2D_SpriteSetCenter(&display, 0.5f, 0.5f);
            C2D_SpriteSetPos(&display, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
            C2D_DrawSprite(&display);
            storeGems.SetText("BartGems: " + std::to_string(gems));
            storeGems.x = SCREEN_WIDTH / 2 - 40;
            storeGems.y = SCREEN_HEIGHT / 2;
            storeGems.Draw();
        }
        if (storephase == 1)
        {
            C2D_DrawSprite(&mainmenuSprites[2]);
            C2D_SpriteSetCenter(&paints[0].sprite, 0.5f, 0.5f);
            paints[0].x = SCREEN_WIDTH / 2;
            paints[0].y = SCREEN_HEIGHT / 2;
            C2D_SpriteSetRotationDegrees(&paints[0].sprite, -25.0f);
            drawCopper();
            copperBartText.Draw();
            C2D_Sprite bartsprite;
            C2D_SpriteFromSheet(&bartsprite, SpriteManager_GetSheet(&spriteManager, "barts"), 3);
            C2D_SpriteSetPos(&bartsprite, 0, 0);
            C2D_DrawSprite(&bartsprite);
            storeGems.SetText("BartGems: " + std::to_string(gems));
            C2D_Sprite display;
            C2D_SpriteFromSheet(&display, SpriteManager_GetSheet(&spriteManager, "UI4"), 0);
            C2D_SpriteSetCenter(&display, 0.5f, 0.5f);
            C2D_SpriteSetPos(&display, SCREEN_WIDTH / 2  + 125, 50);
            C2D_DrawSprite(&display);
            storeGems.x = 290;
            storeGems.y = 55;
            storeGems.Draw();
        }
        else if (storephase == 2)
        {
            C2D_DrawSprite(&mainmenuSprites[2]);
            C2D_SpriteSetCenter(&paints[1].sprite, 0.5f, 0.5f);
            paints[1].x = SCREEN_WIDTH / 2;
            paints[1].y = SCREEN_HEIGHT / 2;
            C2D_SpriteSetRotationDegrees(&paints[1].sprite, -25.0f);
            drawGold();
            goldBartText.Draw();
            C2D_Sprite bartsprite;
            C2D_SpriteFromSheet(&bartsprite, SpriteManager_GetSheet(&spriteManager, "barts"), 4);
            C2D_SpriteSetPos(&bartsprite, 0, 0);
            C2D_DrawSprite(&bartsprite);
            storeGems.SetText("BartGems: " + std::to_string(gems));
            C2D_Sprite display;
            C2D_SpriteFromSheet(&display, SpriteManager_GetSheet(&spriteManager, "UI4"), 0);
            C2D_SpriteSetCenter(&display, 0.5f, 0.5f);
            C2D_SpriteSetPos(&display, SCREEN_WIDTH / 2  + 125, 50);
            C2D_DrawSprite(&display);
            storeGems.x = 290;
            storeGems.y = 55;
            storeGems.Draw();
        }
    }
}

void drawBottom(C3D_RenderTarget *target)
{
    C2D_TargetClear(target, C2D_Color32(0, 0, 0, 1));
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

        player = PhysicsManager_GetPlayer();
        if (player && playerEnabled) // Only draw player if it exists and is enabled
        {
            b2Vec2 pos = player->GetPosition();
            float px = MetersToPixels(pos.x);
            float py = MetersToPixels(pos.y);
            C2D_Sprite playerSprite;

            C2D_SpriteFromSheet(&playerSprite, SpriteManager_GetSheet(&spriteManager, "barts"), 2);
            C2D_SpriteSetPos(&playerSprite, px, py);
            C2D_SpriteSetRotation(&playerSprite, player->GetAngle());
            C2D_SpriteSetCenter(&playerSprite, 0.5f, 0.5f);
            C2D_DrawSprite(&playerSprite);
        }
        UIButton_Update(&itemsButton, touch);
        UIButton_Draw(&itemsButton);
        if (itemsButton.toggled)
        {
            C2D_Sprite copperSprite;
            C2D_SpriteFromSheet(&copperSprite, SpriteManager_GetSheet(&spriteManager, "Copper"), 0);
            C2D_SpriteSetPos(&copperSprite, 60, 0);
            UIButton_Update(&copperPaint, touch);
            UIButton_Draw(&copperPaint);
            C2D_DrawSprite(&copperSprite);
            C2D_Sprite goldSprite;
            C2D_SpriteFromSheet(&goldSprite, SpriteManager_GetSheet(&spriteManager, "Gold"), 0);
            C2D_SpriteSetPos(&goldSprite, 125, 0);
            UIButton_Update(&goldPaint, touch);
            UIButton_Draw(&goldPaint);
            C2D_DrawSprite(&goldSprite);
            copperamount.SetText("(" + std::to_string(copperPaintCount) + ")");
            copperamount.Draw();
            goldamount.SetText("(" + std::to_string(goldPaintCount) + ")");
            goldamount.Draw();
        }
        if (bartphase == 0)
        {
            if (player)
            {
                player->SetTransform(b2Vec2(PixelsToMeters(190), PixelsToMeters(20)), 0);
                player->SetType(b2_staticBody);
            }
            if (kDown & KEY_TOUCH)
            {
                findBart(touch, &selectedBarts, &spriteManager, itemsButton.toggled);
            }

            if (itemsButton.toggled)
            {
                if (copperPaint.toggled && copperPaintCount > 0)
                {
                    paintBart(touch, &spriteManager, false, &copperPaintCount, &goldPaintCount);
                }
                else if (goldPaint.toggled && goldPaintCount > 0)
                {
                    paintBart(touch, &spriteManager, true, &copperPaintCount, &goldPaintCount);
                }
            }
            if (kDown & KEY_A && selectedBarts > 0)
            {
                bartphase = 1;
                PhysicsManager_SpawnPlayer(190, 20);
                playerEnabled = true;
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
            player->SetTransform(b2Vec2(PixelsToMeters(touchX), PixelsToMeters(20)), 0);

            if (kDown & KEY_A)
            {
                player->SetType(b2_dynamicBody);
                bartphase = 2;
                applyRandomUpwardForce(player);
            }
        }
    }
    else if (scenemanager.currentScene == 2)
    {
        C2D_DrawSprite(&mainmenuSprites[2]);
        howtoplayText.Draw();
    }
    else if (scenemanager.currentScene == 4)
    {
        UIButton_Update(&continuebutton, touch);
        UIButton_Draw(&continuebutton);
        UIButton_Update(&storebutton, touch);
        UIButton_Draw(&storebutton);
    }
    else if (scenemanager.currentScene == 5)
    {
        int offsetX = -35;
        if (storephase == 0)
        {

            bool hoveringCopper = touch.px >= offsetX + 156 + 30 && touch.px <= offsetX + 156 + 92 + 30 &&
                                  touch.py >= 80 && touch.py <= 240;

            bool hoveringGold = touch.px >= offsetX + 63 + 30 && touch.px <= offsetX + 155 + 30 &&
                                touch.py >= 80 && touch.py <= 240;
            C2D_Sprite background;
            C2D_SpriteFromSheet(&background, SpriteManager_GetSheet(&spriteManager, "paint"), 0);
            C2D_SpriteSetPos(&background, offsetX, 0);
            C2D_DrawSprite(&background);

            // On touch press
            if (kDown & KEY_TOUCH)
            {
                if (hoveringCopper)
                {
                    holdingCopper = true;
                }
                else if (hoveringGold)
                {
                    holdingGold = true;
                }
            }

            // While touching, update the last valid touch coordinates
            if (kHeld & KEY_TOUCH)
            {
                releaseX = touch.px;
                releaseY = touch.py;
            }

            // On touch release
            if (kUp & KEY_TOUCH)
            {
                // Use cached releaseX/releaseY instead of touch.px/py (which is now 0)
                bool releasedOverCopper = releaseX >= offsetX + 156 + 30 && releaseX <= offsetX + 156 + 92 + 30 &&
                                          releaseY >= 80 && releaseY <= 240;

                bool releasedOverGold = releaseX >= offsetX + 63 + 30 && releaseX <= offsetX + 155 + 30 &&
                                        releaseY >= 80 && releaseY <= 240;

                if (holdingCopper && releasedOverCopper)
                {
                    storephase = 1;
                    fading = true;
                    backgroundOpacity = 1.0f;
                }
                else if (holdingGold && releasedOverGold)
                {
                    storephase = 2;
                    fading = true;
                    backgroundOpacity = 1.0f;
                }

                // Reset state
                holdingCopper = false;
                holdingGold = false;
            }

            C2D_Sprite copperselect;
            C2D_SpriteFromSheet(&copperselect, SpriteManager_GetSheet(&spriteManager, "paint"), 2);
            C2D_SpriteSetPos(&copperselect, offsetX + 96, 0);
            if (hoveringCopper)
                C2D_DrawSprite(&copperselect);

            C2D_Sprite goldselect;
            C2D_SpriteFromSheet(&goldselect, SpriteManager_GetSheet(&spriteManager, "paint"), 1);
            C2D_SpriteSetPos(&goldselect, offsetX + 96, 0);
            if (hoveringGold)
                C2D_DrawSprite(&goldselect);
            UIButton_Update(&backToEnd, touch);
            UIButton_Draw(&backToEnd);
        }
        else if (storephase == 1)
        {
            UIButton_Update(&buyButton, touch);
            UIButton_Draw(&buyButton);
            UIButton_Update(&backButton, touch);
            UIButton_Draw(&backButton);
        }
        else if (storephase == 2)
        {
            UIButton_Update(&buyButton, touch);
            UIButton_Draw(&buyButton);
            UIButton_Update(&backButton, touch);
            UIButton_Draw(&backButton);
        }
    }
}
void updateBarts(float deltaTime, SpriteManager *spriteManager)
{
    for (int i = 0; i < 40; ++i)
    {
        if (!barts[i].initialized)
            continue;
        updateBartFading(&barts[i], spriteManager, deltaTime);
    }
}

int main(int argc, char *argv[])
{
    initSOC();
    romfsInit();
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE * 2);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS * 2);
    C2D_Prepare();

    auto *top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    auto *bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    SpriteManager_Init(&spriteManager);
    SpriteManager_Load(&spriteManager, "Copper", "romfs:/gfx/copperpaint.t3x");
    SpriteManager_Load(&spriteManager, "Gold", "romfs:/gfx/goldpaint.t3x");
    SpriteManager_Load(&spriteManager, "UI1", "romfs:/gfx/UI1.t3x");
    SpriteManager_Load(&spriteManager, "UI2", "romfs:/gfx/UI2.t3x");
    SpriteManager_Load(&spriteManager, "UI3", "romfs:/gfx/backgrounds.t3x");
    SpriteManager_Load(&spriteManager, "logo", "romfs:/gfx/logo.t3x");
    SpriteManager_Load(&spriteManager, "barts", "romfs:/gfx/barts.t3x");
    SpriteManager_Load(&spriteManager, "UI4", "romfs:/gfx/UI4.t3x");
    SpriteManager_Load(&spriteManager, "paint", "romfs:/gfx/paint.t3x");

    loadUI();
    texts();
    loadSprites();

    PhysicsManager_Init();
    spawnBarts();
    initBarts(&spriteManager);
    srand(static_cast<unsigned int>(time(NULL)));
    initPaint();
    AudioManager::Init();
    currentAudioHandle = AudioManager::Play("romfs:/sounds/bort.opus", 1.0f, false, 1.0f, 0.0f);
    AudioManager::PreloadAudio("romfs:/sounds/DOW.opus");
    AudioManager::PreloadAudio("romfs:/sounds/cash.opus");
    AudioManager::PreloadAudio("romfs:/sounds/dsgetpow.opus");
    AudioManager::PreloadAudio("romfs:/sounds/gem.opus");
    AudioManager::PreloadAudio("romfs:/sounds/key.opus");
    while (aptMainLoop())
    {
        DeltaTime_Update();
        float dt = DeltaTime_Get();
        hidScanInput();
        kDown = hidKeysDown();
        kHeld = hidKeysHeld();
        kUp = hidKeysUp();
        if (kDown & KEY_START)
            break;
        hidTouchRead(&touch);
        PhysicsManager_Update(1.0f / 60.0f);
        updateBartsAfterPhysics();
        AudioManager::CleanupFinishedInstances();
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        drawTop(top);
        drawTransition();
        drawBottom(bottom);
        drawTransition();
        counting(&multiplier, player);
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
    C2D_Fini();
    AudioManager::Exit();
    C3D_Fini();
    gfxExit();
    romfsExit();
    socExit();
    return 0;
}
