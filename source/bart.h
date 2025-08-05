#pragma once
#include <3ds.h>
#include <citro2d.h>
#include <iterator>
#include <box2d/box2d.h>
#include <random> // For std::random_device, std::mt19937, std::discrete_distribution
#include <array>  // For std::array
#include "sprite_manager.h"
#include "physics_manager.h"
struct SpriteManager;

enum class BartType
{
    REGULAR_BART,
    DIRT_BART,
    BONUS_BART,
    FAKECOPPER_BART,
    FAKEGOLD_BART,
    COPPER_BART,
    GOLD_BART,
    SUPERCOPPER_BART,
    SUPERGOLD_BART,
    GEM_BART
};

enum class FadeState
{
    None,
    FadingOut,
    FadingIn
};

struct Bart
{
    float x, y;
    BartType type;
    bool touched;
    bool clicked;
    bool initialized = false;
    bool pendingActivation = false; // Add this flag
    C2D_Sprite sprite;
    b2Body *body = nullptr;

    float opacity = 1.0f;
    FadeState fadeState = FadeState::None;
    float fadeTimer = 0.0f;
    bool pendingReset = false;
    bool dissapearing = false;
    float tintAmount;    // Ranges from 0.0 (no tint) to 1.0 (full dark)
    float tintTarget;    // Desired target (0.0 when touched, 0.5 or so when idle)
};

extern Bart barts[40];
extern Bart *firstBart;

extern int roundtimer;
extern int maxtime;
extern int basescore;

void drawBarts();
void initBarts(SpriteManager *spriteManager);
void addBart(float x, float y, BartType type);
void spawnBarts();
void updateBartsAfterPhysics();
void findBart(touchPosition touch, int *selectedBarts, SpriteManager *spriteManager, bool itemsButtonToggled);
void deinitBart(Bart *bart);
void reinitBart(Bart *bart, SpriteManager *spriteManager);
void addMultiplier(int *multiplier, Bart bart);
void resetMultiplier(int *multiplier);
void updateBartFading(Bart* bart, SpriteManager* spriteManager, float deltaTime);
void counting(int *multiplier, b2Body *player);
void resetBarts();
void reInitBarts();
void paintBart(touchPosition touch, SpriteManager *spriteManager, bool gold, int *copperPaintCount, int *goldPaintCount);