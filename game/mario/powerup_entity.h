#pragma once

#include "../../engine/engine.h"

class PowerupEntity : public Entity {
public:
    PowerupEntity(float x, float y, const Props&) {
        tag = "powerup";

        renderer = new ANSIIRenderer(x, y);
        renderer->loadFromFile("sprites/star.ansii");
        renderer->zOrder = 15;

        collider = new ANSIICollider(renderer, this, 2);
        // No physics — floats in place
    }
};
