#include "collision_listener.h"
#include "bart.h"
#include "globals.h"
#include <random>
extern SpriteManager spriteManager;

int roundtimer = 200;
int maxtime = 200;
int basescore = 105;

extern SceneManager scenemanager;

Bart barts[40];
Bart *firstBart = nullptr;

void drawBarts()
{
    std::size_t count = std::size(barts);
    for (int i = 0; i < count; i++)
    {
        if (!barts[i].initialized || !barts[i].sprite.image.tex)
            continue;

        if (barts[i].body)
        {
            b2Vec2 pos = barts[i].body->GetPosition();
            float px = MetersToPixels(pos.x);
            float py = MetersToPixels(pos.y);

            barts[i].x = px;
            barts[i].y = py;

            if (barts[i].dissapearing)
            {
                barts[i].opacity -= DeltaTime_Get() * 3.0f;
                if (barts[i].opacity <= 0.0f)
                {
                    barts[i].body->DestroyFixture(barts[i].body->GetFixtureList());
                    barts[i].initialized = false;
                    barts[i].dissapearing = false; // Stop dissapearing
                    continue;                      // Skip drawing this Bart
                }
            }

            C2D_SpriteSetPos(&barts[i].sprite, px, py);
            C2D_SpriteSetRotation(&barts[i].sprite, barts[i].body->GetAngle());
        }

        C2D_ImageTint tint;
        C2D_AlphaImageTint(&tint, barts[i].opacity);
        C2D_DrawSpriteTinted(&barts[i].sprite, &tint);
    }
}
void initBarts(SpriteManager *spriteManager)
{
    std::size_t count = std::size(barts);
    b2World *world = PhysicsManager_GetWorld();
    for (int i = 0; i < count; i++)
    {
        if (barts[i].initialized)
        {
            printf("Initializing Bart %d at x=%f y=%f type=%d\n", i, barts[i].x, barts[i].y, static_cast<int>(barts[i].type));
            // Create Box2D body for Bart
            b2BodyDef def;
            def.type = b2_dynamicBody;
            def.position.Set(PixelsToMeters(barts[i].x), PixelsToMeters(barts[i].y));
            def.linearVelocity.Set(0.0f, 0.0f);
            barts[i].body = world->CreateBody(&def);

            b2PolygonShape shape;
            shape.SetAsBox(PixelsToMeters(7), PixelsToMeters(15));
            b2FixtureDef fix;
            fix.shape = &shape;
            fix.density = 0.5f;     // Very low mass
            fix.friction = 0.0f;    // Very slippery
            fix.restitution = 0.8f; // Very bouncy
            barts[i].body->CreateFixture(&fix);

            barts[i].body->SetGravityScale(0.0f);   // Prevent falling into a pile
            barts[i].body->SetLinearDamping(1.0f);  // Reduce movement after spawn
            barts[i].body->SetAngularDamping(2.0f); // Strongly damp rotation

            int spriteIndex = static_cast<int>(barts[i].type);
            printf("Bart %d: SpriteSheet=%p, SpriteIndex=%d\n", i, SpriteManager_GetSheet(spriteManager, "barts"), spriteIndex);

            C2D_SpriteFromSheet(&barts[i].sprite, SpriteManager_GetSheet(spriteManager, "barts"), spriteIndex);
            C2D_SpriteSetCenter(&barts[i].sprite, 0.5f, 0.5f);
            C2D_SpriteSetPos(&barts[i].sprite, barts[i].x, barts[i].y);
        }
    }
}

void addBart(float x, float y, BartType type)
{
    std::size_t count = std::size(barts);
    for (int i = 0; i < count; i++)
    {
        if (!barts[i].initialized)
        {
            barts[i].x = x;
            barts[i].y = y;
            barts[i].type = type;
            barts[i].touched = false;
            barts[i].clicked = false;
            barts[i].initialized = true;
            return;
        }
    }
}

struct BartChance
{
    BartType type;
    float weight;
};

BartType getRandomBartType()
{
    static std::mt19937 rng(std::random_device{}());
    static std::vector<BartChance> chances = {
        {BartType::REGULAR_BART, 0.50f},
        {BartType::DIRT_BART, 0.10f},
        {BartType::GOLD_BART, 0.05f},
        {BartType::COPPER_BART, 0.08f},
        {BartType::SUPERGOLD_BART, 0.01f},
        {BartType::SUPERCOPPER_BART, 0.02f},
        {BartType::GEM_BART, 0.03f},
        {BartType::FAKECOPPER_BART, 0.10f},
        {BartType::FAKEGOLD_BART, 0.11f}};

    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float roll = dist(rng);

    float cumulative = 0.0f;
    for (const auto &c : chances)
    {
        cumulative += c.weight;
        if (roll < cumulative)
            return c.type;
    }

    return BartType::REGULAR_BART;
}

void spawnBarts()
{
    const std::size_t count = std::size(barts);
    const float minDistance = 25.0f;
    const int maxAttemptsPerBart = 100;

    // Random number generator setup
    static std::random_device rd;                   // Seed source (once)
    static std::mt19937 gen(rd());                  // Mersenne Twister RNG
    std::uniform_int_distribution<> distX(40, 279); // 240 + 40 - 1 = 279
    std::uniform_int_distribution<> distY(80, 219); // 140 + 80 - 1 = 219

    for (int i = 0; i < count; i++)
    {
        int attempts = 0;
        float x = 0.0f, y = 0.0f;

        bool positionOkay = false;
        while (attempts < maxAttemptsPerBart && !positionOkay)
        {
            x = static_cast<float>(distX(gen));
            y = static_cast<float>(distY(gen));
            positionOkay = true;

            for (int j = 0; j < i; ++j)
            {
                if (!barts[j].initialized)
                    continue;

                float dx = x - barts[j].x;
                float dy = y - barts[j].y;
                float distSq = dx * dx + dy * dy;
                if (distSq < minDistance * minDistance)
                {
                    positionOkay = false;
                    break; // Too close, try again
                }
            }

            attempts++;
        }

        if (positionOkay)
        {
            BartType type = getRandomBartType();
            addBart(x, y, type);
        }
        else
        {
            printf("Could not find good position for Bart %d\n", i);
        }
    }
}

void updateBartsAfterPhysics()
{
    for (int i = 0; i < 40; ++i)
    {
        if (barts[i].pendingActivation && barts[i].body->GetType() == b2_staticBody)
        {
            // Remove old fixtures
            b2Fixture *f = barts[i].body->GetFixtureList();
            while (f)
            {
                b2Fixture *next = f->GetNext();
                barts[i].body->DestroyFixture(f);
                f = next;
            }
            // Change to dynamic
            barts[i].body->SetType(b2_dynamicBody);

            // Create new fixture with non-zero density
            b2PolygonShape shape;
            shape.SetAsBox(PixelsToMeters(8), PixelsToMeters(15));
            b2FixtureDef fix;
            fix.shape = &shape;
            fix.density = 0.2f;
            fix.friction = 0.8f;
            fix.restitution = 0.9f;
            barts[i].body->CreateFixture(&fix);

            barts[i].body->SetLinearDamping(1.0f);
            barts[i].body->SetGravityScale(0.0f);

            barts[i].pendingActivation = false; // Clear flag
        }
    }
}

void findBart(touchPosition touch, int *selectedBarts, SpriteManager *spriteManager, bool itemsButtonToggled)
{
    b2Vec2 testPoint = b2Vec2(PixelsToMeters(touch.px), PixelsToMeters(touch.py));
    for (int i = 0; i < 40; ++i)
    {
        if (!barts[i].initialized || !barts[i].body || !(barts[i].type == BartType::REGULAR_BART || barts[i].type == BartType::BONUS_BART))
            continue;
        for (b2Fixture *f = barts[i].body->GetFixtureList(); f; f = f->GetNext())
        {
            if (f->TestPoint(testPoint))
            {
                if (barts[i].type == BartType::REGULAR_BART && !barts[i].clicked && *selectedBarts < 6 && !itemsButtonToggled)
                {
                    // Select it
                    barts[i].clicked = true;
                    barts[i].type = BartType::BONUS_BART;
                    barts[i].sprite.image = C2D_SpriteSheetGetImage(SpriteManager_GetSheet(spriteManager, "barts"), static_cast<int>(barts[i].type));
                    if (*selectedBarts == 0)
                    {
                        firstBart = &barts[i];
                    }
                    AudioManager::Play("romfs:/sounds/dsgetpow.opus", 1.0f + (*selectedBarts / 15.0f), false, 1.0f, 0.0f);

                    (*selectedBarts)++;
                }
                else if (barts[i].type == BartType::BONUS_BART && barts[i].clicked)
                {
                    // Deselect it
                    barts[i].clicked = false;
                    barts[i].type = BartType::REGULAR_BART;
                    barts[i].sprite.image = C2D_SpriteSheetGetImage(SpriteManager_GetSheet(spriteManager, "barts"), static_cast<int>(barts[i].type));
                    (*selectedBarts)--;
                }
            }
        }
    }
}

void paintBart(touchPosition touch, SpriteManager *spriteManager, bool gold, int *copperPaintCount, int *goldPaintCount)
{
    b2Vec2 testPoint = b2Vec2(PixelsToMeters(touch.px), PixelsToMeters(touch.py));
    for (int i = 0; i < 40; ++i)
    {
        if (!barts[i].initialized || !barts[i].body || !(barts[i].type == BartType::REGULAR_BART || barts[i].type == BartType::BONUS_BART))
            continue;
        for (b2Fixture *f = barts[i].body->GetFixtureList(); f; f = f->GetNext())
        {
            if (f->TestPoint(testPoint))
            {
                if (barts[i].type == BartType::REGULAR_BART && !barts[i].clicked && !gold)
                {
                    barts[i].type = BartType::FAKECOPPER_BART;
                    barts[i].sprite.image = C2D_SpriteSheetGetImage(SpriteManager_GetSheet(spriteManager, "barts"), static_cast<int>(barts[i].type));
                    (*copperPaintCount)--;
                }
                else if (barts[i].type == BartType::REGULAR_BART && !barts[i].clicked && gold)
                {
                    barts[i].type = BartType::FAKEGOLD_BART;
                    barts[i].sprite.image = C2D_SpriteSheetGetImage(SpriteManager_GetSheet(spriteManager, "barts"), static_cast<int>(barts[i].type));
                    (*goldPaintCount)--;
                }
            }
        }
    }
}

void deinitBart(Bart *bart)
{
    bart->initialized = false;
    if (bart->body)
    {
        PhysicsManager_GetWorld()->DestroyBody(bart->body);
        bart->body = nullptr;
    }
    // Optionally hide sprite or set to a blank image
    bart->sprite.image = C2D_Image(); // Set to empty image if available
}

void reinitBart(Bart *bart, SpriteManager *spriteManager)
{
    bart->initialized = true;
    // Restore Box2D body
    b2World *world = PhysicsManager_GetWorld();
    b2BodyDef def;
    def.type = b2_dynamicBody;
    def.position.Set(PixelsToMeters(bart->x), PixelsToMeters(bart->y));
    def.linearVelocity.Set(0.0f, 0.0f);
    bart->body = world->CreateBody(&def);

    b2PolygonShape shape;
    shape.SetAsBox(PixelsToMeters(7), PixelsToMeters(15));
    b2FixtureDef fix;
    fix.shape = &shape;
    fix.density = 0.75f;
    fix.friction = 0.2f;
    fix.restitution = 1.0f;
    bart->body->CreateFixture(&fix);

    bart->body->SetGravityScale(0.0f);
    bart->body->SetLinearDamping(1.0f);
    bart->body->SetAngularDamping(20.0f);

    // Restore sprite
    int spriteIndex = static_cast<int>(bart->type);
    bart->sprite.image = C2D_SpriteSheetGetImage(SpriteManager_GetSheet(spriteManager, "barts"), spriteIndex);
    C2D_SpriteSetCenter(&bart->sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&bart->sprite, bart->x, bart->y);
}

void addMultiplier(int *multiplier, Bart bart)
{
    if (bart.touched)
        return; // No multiplier if not touched
    if (bart.type == BartType::REGULAR_BART)
    {
        *multiplier += 2;
    }
    else if (bart.type == BartType::DIRT_BART)
    {
        *multiplier += 1;
    }
    else if (bart.type == BartType::FAKECOPPER_BART)
    {
        *multiplier += 5;
    }
    else if (bart.type == BartType::FAKEGOLD_BART)
    {
        *multiplier += 7;
    }
    else if (bart.type == BartType::COPPER_BART)
    {
        *multiplier += 8;
    }
    else if (bart.type == BartType::GOLD_BART)
    {
        *multiplier += 16;
    }
    else if (bart.type == BartType::SUPERCOPPER_BART)
    {
        *multiplier += 32;
    }
    else if (bart.type == BartType::SUPERGOLD_BART)
    {
        *multiplier += 64;
    }
    else if (bart.type == BartType::BONUS_BART)
    {
        *multiplier += 4;
    }
}

void resetMultiplier(int *multiplier)
{
    *multiplier = 1; // Reset to 1x
}

void counting(int *multiplier, b2Body *player)
{
    if (startcounting && bartphase == 2)
    {
        roundtimer -= DeltaTime_Get();
        if (roundtimer <= 0)
        {
            score = basescore * *multiplier;
            totalScore += score;
            *multiplier = 1;
            player->SetTransform(b2Vec2(PixelsToMeters(190), PixelsToMeters(20)), 0);
            player->SetType(b2_staticBody);
            *playerEnabledPtr = false;
            roundtimer = maxtime;  // Reset timer for next round
            startcounting = false; // Stop counting until next round
            bartphase = 0;
            selectedBarts = 0;
            resetBarts();

            (*currentRoundPtr) += 1;
            if (*currentRoundPtr >= 3)
            {
                changeScene(&scenemanager, 4);
            }
        }
    }
    else
    {
        roundtimer = maxtime;
    }
}

void updateBartFading(Bart *bart, SpriteManager *spriteManager, float deltaTime)
{
    const float fadeDuration = 0.3f; // seconds

    if (bart->fadeState == FadeState::FadingOut)
    {
        bart->fadeTimer += deltaTime;
        bart->opacity = 1.0f - (bart->fadeTimer / fadeDuration);

        if (bart->opacity <= 0.0f)
        {
            bart->opacity = 0.0f;
            bart->fadeState = FadeState::FadingIn;
            bart->fadeTimer = 0.0f;

            if (bart->pendingReset)
            {
                deinitBart(bart);
                reinitBart(bart, spriteManager);
                bart->pendingReset = false;
            }
        }
    }
    else if (bart->fadeState == FadeState::FadingIn)
    {
        bart->fadeTimer += deltaTime;
        bart->opacity = bart->fadeTimer / fadeDuration;

        if (bart->opacity >= 1.0f)
        {
            bart->opacity = 1.0f;
            bart->fadeState = FadeState::None;
            bart->fadeTimer = 0.0f;
        }
    }
}

void resetBarts()
{
    for (int i = 0; i < 40; ++i)
    {
        if (barts[i].initialized)
        {
            deinitBart(&barts[i]);
        }
    }
    spawnBarts();  // Generates new positions
    reInitBarts(); // Reinitialize barts with new positions
}

void reInitBarts()
{
    for (int i = 0; i < 40; ++i)
    {
        if (barts[i].initialized && !barts[i].body)
        {
            reinitBart(&barts[i], &spriteManager);
        }
    }
}