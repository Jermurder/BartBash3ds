#include "collision_listener.h"
#include "bart.h"
#include "physics_manager.h"

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