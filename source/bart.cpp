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
            barts[i].body = world->CreateBody(&def);

            b2PolygonShape shape;
            shape.SetAsBox(PixelsToMeters(8), PixelsToMeters(15)); // 32x32 px box.
            b2FixtureDef fix;
            fix.shape = &shape;
            fix.density = 5.0f;    // Lower density for easier movement (tweak as needed)
            fix.friction = 0.8f;     // Higher friction for less sliding
            fix.restitution = 0.05f;
            barts[i].body->CreateFixture(&fix);

            barts[i].body->SetLinearDamping(1.0f);
            barts[i].body->SetGravityScale(0.0f);

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