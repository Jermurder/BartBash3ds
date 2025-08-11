#include "physics_manager.h"
#include <cmath>

#define PIXELS_PER_METER 60.0f
#define METERS_PER_PIXEL (1.0f / PIXELS_PER_METER)

b2World *world = nullptr;
static b2Body *playerBody = nullptr;
static CollisionListener collisionListener;

float PixelsToMeters(float px) { return px * METERS_PER_PIXEL; }
float MetersToPixels(float m) { return m * PIXELS_PER_METER; }

bool isFrozen = false;

void PhysicsManager_Init()
{
    b2Vec2 gravity(0.0f, 9.8f);
    world = new b2World(gravity);

    // Ground
    b2BodyDef groundDef;
    groundDef.position.Set(PixelsToMeters(200), PixelsToMeters(240)); // center bottom
    b2Body *ground = world->CreateBody(&groundDef);

    b2PolygonShape groundShape;
    groundShape.SetAsBox(PixelsToMeters(320) / 2.0f, PixelsToMeters(10)); // full width floor
    ground->CreateFixture(&groundShape, 0.0f);

    // Left wall
    b2BodyDef leftWallDef;
    leftWallDef.position.Set(PixelsToMeters(30), PixelsToMeters(120)); // 40px from left, vertically centered
    b2Body *leftWall = world->CreateBody(&leftWallDef);

    b2PolygonShape leftWallShape;
    leftWallShape.SetAsBox(PixelsToMeters(10), PixelsToMeters(120)); // 20px wide, 240px tall
    leftWall->CreateFixture(&leftWallShape, 0.0f);

    // Right wall
    b2BodyDef rightWallDef;
    rightWallDef.position.Set(PixelsToMeters(320 - 30), PixelsToMeters(120)); // 40px from right, vertically centered
    b2Body *rightWall = world->CreateBody(&rightWallDef);

    b2PolygonShape rightWallShape;
    rightWallShape.SetAsBox(PixelsToMeters(10), PixelsToMeters(120)); // 20px wide, 240px tall
    rightWall->CreateFixture(&rightWallShape, 0.0f);

    world->SetContactListener(&collisionListener);
}

void PhysicsManager_Update(float dt)
{
    world->Step(dt, 6, 2);
}

void PhysicsManager_SpawnPlayer(float screenX, float screenY)
{
    if (playerBody)
    {
        world->DestroyBody(playerBody);
        playerBody = nullptr;
    }

    b2BodyDef def;
    def.type = b2_dynamicBody;
    def.position.Set(PixelsToMeters(screenX), PixelsToMeters(screenY));
    playerBody = world->CreateBody(&def);

    // Capsule dimensions
    float halfWidth = PixelsToMeters(3);   // radius for circles, half-width for box
    float halfHeight = PixelsToMeters(15); // half-height for box

    // Rectangle (center)
    b2PolygonShape box;
    box.SetAsBox(halfWidth, halfHeight);

    // Top circle
    b2CircleShape topCircle;
    topCircle.m_radius = halfWidth;
    topCircle.m_p.Set(0, -halfHeight);

    // Bottom circle
    b2CircleShape bottomCircle;
    bottomCircle.m_radius = halfWidth;
    bottomCircle.m_p.Set(0, halfHeight);

    b2FixtureDef fix;
    fix.density = 0.75f;
    fix.friction = 0.1f;
    fix.restitution = 0.7f;

    // playerBody->SetLinearDamping(1.0f); // Less friction than barts

    // Attach all three shapes to the player body
    fix.shape = &box;
    playerBody->CreateFixture(&fix);

    fix.shape = &topCircle;
    playerBody->CreateFixture(&fix);

    fix.shape = &bottomCircle;
    playerBody->CreateFixture(&fix);

    playerBody->SetGravityScale(0.8f); // Falls at half normal speed
}

b2Body *PhysicsManager_GetPlayer()
{
    return playerBody;
}

b2World *PhysicsManager_GetWorld()
{
    return world;
}

void PhysicsManager_TogglePlayerFrozen()
{
    if (!playerBody)
        return;

    isFrozen = !isFrozen;

    if (isFrozen)
    {
        playerBody->SetGravityScale(0.0f);
        playerBody->SetLinearVelocity(b2Vec2_zero);
        playerBody->SetAngularVelocity(0.0f);
        playerBody->SetAwake(false);
    }
    else
    {
        playerBody->SetGravityScale(1.0f);
        playerBody->SetAwake(true);
    }
}


void applyRandomUpwardForce(b2Body* body)
{
    // Random engine setup
    static std::random_device rd;
    static std::mt19937 gen(rd());

    // Horizontal force range, e.g., -5 to +5
    std::uniform_real_distribution<float> distX(-0.05f, 0.05f);

    // Fixed upward force
    float upwardForce = -0.05f;

    // Create force vector
    b2Vec2 force(distX(gen), upwardForce);

    // Apply the force to the center of mass
    body->ApplyLinearImpulseToCenter(force, true);
}
