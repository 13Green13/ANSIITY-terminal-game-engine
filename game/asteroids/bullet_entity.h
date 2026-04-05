#pragma once

#include "../../engine/engine.h"
#include "../../engine/constants.h"

class BulletEntity : public Entity {
    float _velX, _velY;
    float _lifetime;

public:
    BulletEntity(float x, float y, float vx, float vy)
        : _velX(vx), _velY(vy), _lifetime(2.0f)
    {
        tag = "bullet";

        renderer = new ANSIIRenderer(x, y);
        renderer->loadFromFile("sprites/ship_bullet.ansii");
        renderer->zOrder = 25;

        collider = new ANSIICollider(renderer, this, 1);
        collider->layerMask[0] = false;  // don't collide with ship
        collider->layerMask[1] = false;  // don't collide with other bullets
    }

    void update(float dt) override {
        renderer->x += _velX * dt;
        renderer->y += _velY * dt;

        _lifetime -= dt;

        // Die if off-screen or expired
        if (_lifetime <= 0 ||
            renderer->x < -1 || renderer->x > WIDTH + 1 ||
            renderer->y < -1 || renderer->y > HEIGHT + 1) {
            destroy();
        }
    }

    void onCollision(Entity* other, const CollisionInfo& info) override {
        if (other->tag == "asteroid") {
            destroy();
        }
    }
};
