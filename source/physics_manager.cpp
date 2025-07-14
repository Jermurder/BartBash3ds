#include "physics_manager.h"
#include <cmath>

#define PIXELS_PER_METER 60.0f
#define METERS_PER_PIXEL (1.0f / PIXELS_PER_METER)

static b2World* world = nullptr;
static b2Body* playerBody = nullptr;

float PixelsToMeters(float px) { return px * METERS_PER_PIXEL; }
float MetersToPixels(float m)  { return m * PIXELS_PER_METER; }

void PhysicsManager_Init() {
    b2Vec2 gravity(0.0f, 9.8f);
    world = new b2World(gravity);

    // Ground
    b2BodyDef groundDef;
    groundDef.position.Set(PixelsToMeters(200), PixelsToMeters(240)); // center bottom
    b2Body* ground = world->CreateBody(&groundDef);

    b2PolygonShape groundShape;
    groundShape.SetAsBox(PixelsToMeters(320) / 2.0f, PixelsToMeters(10)); // full width floor
    ground->CreateFixture(&groundShape, 0.0f);

    // Left wall
    b2BodyDef leftWallDef;
    leftWallDef.position.Set(PixelsToMeters(40), PixelsToMeters(120)); // 40px from left, vertically centered
    b2Body* leftWall = world->CreateBody(&leftWallDef);

    b2PolygonShape leftWallShape;
    leftWallShape.SetAsBox(PixelsToMeters(10), PixelsToMeters(120)); // 20px wide, 240px tall
    leftWall->CreateFixture(&leftWallShape, 0.0f);

    // Right wall
    b2BodyDef rightWallDef;
    rightWallDef.position.Set(PixelsToMeters(400 - 40), PixelsToMeters(120)); // 40px from right, vertically centered
    b2Body* rightWall = world->CreateBody(&rightWallDef);

    b2PolygonShape rightWallShape;
    rightWallShape.SetAsBox(PixelsToMeters(10), PixelsToMeters(120)); // 20px wide, 240px tall
    rightWall->CreateFixture(&rightWallShape, 0.0f);
}

void PhysicsManager_Update(float dt) {
    world->Step(dt, 6, 2);
}

void PhysicsManager_SpawnPlayer(float screenX, float screenY) {
    if (playerBody) {
        world->DestroyBody(playerBody);
        playerBody = nullptr;
    }

    b2BodyDef def;
    def.type = b2_dynamicBody;
    def.position.Set(PixelsToMeters(screenX), PixelsToMeters(screenY));
    playerBody = world->CreateBody(&def);

    b2PolygonShape shape;
    shape.SetAsBox(PixelsToMeters(10), PixelsToMeters(20)); // 40x40 px box

    b2FixtureDef fix;
    fix.shape = &shape;
    fix.density = 2.0f;
    fix.friction = 0.3f;
    fix.restitution = 0.2f;
    playerBody->CreateFixture(&fix);
    // After creating the player body
    playerBody->SetGravityScale(0.5f); // Falls at half normal speed
}

b2Body* PhysicsManager_GetPlayer() {
    return playerBody;
}

b2World* PhysicsManager_GetWorld() {
    return world;
}

bool isFrozen = false;

void PhysicsManager_TogglePlayerFrozen() {
    if (!playerBody) return;

    isFrozen = !isFrozen;

    if (isFrozen) {
        playerBody->SetGravityScale(0.0f);
        playerBody->SetLinearVelocity(b2Vec2_zero);
        playerBody->SetAngularVelocity(0.0f);
        playerBody->SetAwake(false);
    } else {
        playerBody->SetGravityScale(1.0f);
        playerBody->SetAwake(true);
    }
}