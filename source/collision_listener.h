#pragma once
#include <box2d/box2d.h>
#include "globals.h"

class CollisionListener : public b2ContactListener
{
public:
    void BeginContact(b2Contact *contact) override;
};
