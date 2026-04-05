#pragma once

#include "../../engine/engine.h"

class FinishEntity : public Entity {
public:
    FinishEntity(float x, float y, const Props&) {
        tag = "finish";

        renderer = new ANSIIRenderer(x, y);
        renderer->loadFromFile("sprites/finish.ansii");
        renderer->zOrder = 15;

        collider = new ANSIICollider(renderer, this, 3);
        // No physics — static flag
    }
};
