#include "collision_listener.h"
#include "bart.h"
#include "physics_manager.h"
#include "audio_manager.h"
#include <random>
void CollisionListener::BeginContact(b2Contact *contact)
{
    b2Body *bodyA = contact->GetFixtureA()->GetBody();
    b2Body *bodyB = contact->GetFixtureB()->GetBody();

    b2Body *player = PhysicsManager_GetPlayer();
    for (int i = 0; i < 40; ++i)
    {
        if (!barts[i].initialized || !barts[i].body)
            continue;

        if ((bodyA == player && bodyB == barts[i].body) ||
            (bodyB == player && bodyA == barts[i].body))
        {
            if (barts[i].body->GetType() == b2_staticBody)
            {
                barts[i].pendingActivation = true; // Set flag, don't activate here!
            }
            addMultiplier(&multiplier, barts[i]);
            if (barts[i].touched == false)
            {
                roundtimer = maxtime;
                static std::random_device rd;
                static std::mt19937 gen(rd());
                static std::uniform_real_distribution<float> pitchDist(0.6f, 1.4f);

                float randomPitch = pitchDist(gen);

                AudioManager::Play("romfs:/sounds/DOW.opus", randomPitch, false, 1.0f, 0.0f);
            }

            if (barts[i].type == BartType::GEM_BART && barts[i].touched == false)
            {
                gems++;
                barts[i].dissapearing = true; // Mark for dissapear
            }
            barts[i].touched = true;
            startcounting = true;
        }
    }
}