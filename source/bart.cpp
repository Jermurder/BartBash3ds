#include "collision_listener.h"
#include "bart.h"
#include "sprite_manager.h"

Bart barts[40];

void drawBarts()
{
    std::size_t count = std::size(barts);
    for (int i = 0; i < count; i++)
    {
        if (!barts[i].initialized) {
            printf("Bart %d not initialized\n", i);
            continue;
        }
        if (barts[i].body) {
            b2Vec2 pos = barts[i].body->GetPosition();
            float px = MetersToPixels(pos.x);
            float py = MetersToPixels(pos.y);

            printf("Bart %d: px=%f py=%f type=%d\n", i, px, py, static_cast<int>(barts[i].type));

            barts[i].x = px;
            barts[i].y = py;

            C2D_SpriteSetPos(&barts[i].sprite, px, py);
            C2D_SpriteSetRotation(&barts[i].sprite, barts[i].body->GetAngle());
        } else {
            printf("Bart %d has no body\n", i);
        }
        C2D_DrawSprite(&barts[i].sprite);
    }
}

void initBarts(SpriteManager *spriteManager)
{
    std::size_t count = std::size(barts);
    b2World* world = PhysicsManager_GetWorld();
    for (int i = 0; i < count; i++)
    {
        if (barts[i].initialized)
        {
            printf("Initializing Bart %d at x=%f y=%f type=%d\n", i, barts[i].x, barts[i].y, static_cast<int>(barts[i].type));
            // Create Box2D body for Bart
            b2BodyDef def;
            def.type = b2_dynamicBody;
            def.position.Set(PixelsToMeters(barts[i].x), PixelsToMeters(barts[i].y));
            def.linearVelocity.Set(0.0f, 0.0f); // Start still
            barts[i].body = world->CreateBody(&def);

            b2PolygonShape shape;
            shape.SetAsBox(PixelsToMeters(7), PixelsToMeters(15)); // 32x32 px box.
            b2FixtureDef fix;
            fix.shape = &shape; // See capsule below
            fix.density = 0.05f;      // Very low mass
            fix.friction = 0.05f;    // Very slippery
            fix.restitution = 0.9f;  // Very bouncy
            barts[i].body->CreateFixture(&fix);

            barts[i].body->SetGravityScale(0.0f);  // Prevent falling into a pile
            barts[i].body->SetLinearDamping(4.0f); // Reduce movement after spawn

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

BartType getRandomBartType() {
    // NOTE: Index 2 (BONUS_BART) is excluded.
    static const std::array<BartType, 9> types = {
        BartType::REGULAR_BART,
        BartType::DIRT_BART,
        BartType::FAKECOPPER_BART,
        BartType::FAKEGOLD_BART,
        BartType::COPPER_BART,
        BartType::GOLD_BART,
        BartType::SUPERCOPPER_BART,
        BartType::SUPERGOLD_BART,
        BartType::GEM_BART
    };

    static const std::array<int, 9> weights = {
        49, // REGULAR_BART
        14, // DIRT_BART
        10, // FAKECOPPER_BART
        4,  // FAKEGOLD_BART
        2,  // COPPER_BART
        2,  // GOLD_BART
        2,  // SUPERCOPPER_BART
        2,  // SUPERGOLD_BART
        2   // GEM_BART
    };

    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::discrete_distribution<> dist(weights.begin(), weights.end());

    int index = dist(gen);
    return types[index];
}

void spawnBarts()
{
    std::size_t count = std::size(barts);
    for (int i = 0; i < count; i++)
    {
        int x = rand() % 290 + 30;
        int y = rand() % 140 + 100;

        BartType type = getRandomBartType();
        addBart(x, y, type);
    }
}

void updateBartsAfterPhysics() {
    for (int i = 0; i < 40; ++i) {
        if (barts[i].pendingActivation && barts[i].body->GetType() == b2_staticBody) {
            // Remove old fixtures
            b2Fixture* f = barts[i].body->GetFixtureList();
            while (f) {
                b2Fixture* next = f->GetNext();
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
            fix.density = 1.0f;
            fix.friction = 0.8f;
            fix.restitution = 0.05f;
            barts[i].body->CreateFixture(&fix);

            barts[i].body->SetLinearDamping(1.0f);
            barts[i].body->SetGravityScale(0.0f);

            barts[i].pendingActivation = false; // Clear flag
        }
    }
}
