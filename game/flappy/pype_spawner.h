#pragma once

#include "../../engine/engine.h"
#include "pype_entity.h"

constexpr int PYPE_HEIGHT = 15;

class PypeSpawner : public Entity {
    float _spawnTimer = 40.0f;

public:
    PypeSpawner() {
        // No renderer, no collider — pure logic entity
    }

    void update(float dt) override {
        _spawnTimer += dt * TARGET_FPS;

        if (_spawnTimer > 40.0f) {
            _spawnTimer = 0;

            if (rand() % 2 == 0) {
                auto* p = new PypeEntity(PYPE_HEIGHT);
                p->init();
            } else {
                auto* p1 = new PypeEntity(PYPE_HEIGHT);
                p1->init();
                auto* p2 = new PypeEntity(PYPE_HEIGHT);
                p2->init();
            }
        }
    }
};
