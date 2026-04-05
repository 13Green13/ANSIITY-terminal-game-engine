#pragma once

#include <string>
#include "ansii_renderer.h"
#include "ansii_collider.h"
#include "physics_body.h"

struct CollisionInfo {
    float normalX, normalY;     // direction of collision surface relative to this entity
    float preVelX, preVelY;     // this entity's velocity before physics resolved
    float otherPreVelX, otherPreVelY;  // other entity's velocity before physics resolved
};

class Entity {
public:
    bool alive = true;
    std::string tag;
    ANSIIRenderer* renderer = nullptr;
    ANSIICollider* collider = nullptr;
    PhysicsBody* physics = nullptr;

    virtual ~Entity() {
        delete renderer;
        delete collider;
        delete physics;
    }

    virtual void update(float dt) {}
    virtual void onCollision(Entity* other, const CollisionInfo& info) {}
    virtual void onTileCollision(const TileCollision& tc) {}

    void init();    // defined after managers are declared (in engine.h)
    void destroy() { alive = false; }
};
