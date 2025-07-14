#pragma once
#include <3ds.h>
#include <citro2d.h>
#include <iterator>
#include <box2d/box2d.h>
#include <random>  // For std::random_device, std::mt19937, std::discrete_distribution
#include <array>   // For std::array
struct SpriteManager;

enum class BartType
{
    REGULAR_BART, DIRT_BART, BONUS_BART, FAKECOPPER_BART,
    FAKEGOLD_BART, COPPER_BART, GOLD_BART, SUPERCOPPER_BART,
    SUPERGOLD_BART, GEM_BART};

struct Bart
{
    float x, y;
    BartType type;
    bool touched;
    bool clicked;
    bool initialized = false;
    bool pendingActivation = false; // Add this flag
    C2D_Sprite sprite;
    b2Body* body = nullptr;
};

extern Bart barts[40];

void drawBarts();
void initBarts(SpriteManager *spriteManager);
void addBart(float x, float y, BartType type);
void spawnBarts();
void updateBartsAfterPhysics();