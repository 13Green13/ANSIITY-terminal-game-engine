#pragma once

#include "../../engine/engine.h"
#include "counter_entity.h"
#include <cstdlib>

class CounterEntity;

class PypeEntity : public Entity {
public:
    PypeEntity(int pipeHeight) {
        float startX = (float)(WIDTH - 1);
        float startY;
        if (rand() % 2 == 0) {
            startY = 0;
        } else {
            startY = (float)(HEIGHT - pipeHeight);
        }

        renderer = new ANSIIRenderer(startX, startY);
        renderer->zOrder = 10;
        renderer->loadFromFile("sprites/pype.ansii");
        collider = new ANSIICollider(renderer, this, 1);
        // Pypes don't collide with each other (layer 1 ignores layer 1)
        collider->layerMask[1] = false;
    }

    void update(float dt) override {
        float scale = dt * TARGET_FPS;
        renderer->x -= 1.0f * scale;

        if (renderer->x + renderer->width <= 0) {
            destroy();
            auto* counter = CounterEntity::instance();
            if (counter) counter->incrementScore();
        }
    }
};
