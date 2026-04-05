#pragma once

#include "../../engine/engine.h"
#include "../../engine/entity_manager.h"
#include "../../engine/constants.h"
#include "asteroid_entity.h"
#include "game_state.h"
#include <cstdlib>
#include <cmath>

class SpawnerEntity : public Entity {
    int _asteroidsPerWave = 4;
    float _spawnDelay = 2.0f;   // grace period between waves
    float _timer = 1.0f;       // initial delay before first wave

    static float randFloat(float lo, float hi) {
        return lo + (float)rand() / RAND_MAX * (hi - lo);
    }

    int countAsteroids() {
        int count = 0;
        for (auto* e : EntityManager::getInstance().getEntities()) {
            if (e->alive && e->tag == "asteroid") count++;
        }
        return count;
    }

    void spawnWave() {
        GameState::wave++;
        int count = _asteroidsPerWave + (GameState::wave - 1) * 2;

        for (int i = 0; i < count; i++) {
            // Spawn on screen edges, away from center
            float x, y;
            int edge = rand() % 4;
            switch (edge) {
                case 0: x = randFloat(0, (float)WIDTH);  y = 0;                  break;  // top
                case 1: x = randFloat(0, (float)WIDTH);  y = (float)(HEIGHT - 5); break; // bottom
                case 2: x = 0;                           y = randFloat(0, (float)HEIGHT); break; // left
                default: x = (float)(WIDTH - 5);         y = randFloat(0, (float)HEIGHT); break; // right
            }

            float angle = randFloat(0, 6.2832f);
            float speed = randFloat(15.0f, 35.0f);
            float vx = std::cos(angle) * speed;
            float vy = std::sin(angle) * speed;

            auto* ast = new AsteroidEntity(x, y, AsteroidSize::LARGE, vx, vy);
            ast->init();
        }
    }

public:
    SpawnerEntity(float x, float y, const Props&) {
        tag = "spawner";
        // No renderer, no collider — invisible manager entity
    }

    void update(float dt) override {
        _timer -= dt;
        if (_timer > 0) return;

        if (countAsteroids() == 0) {
            _timer = _spawnDelay;
            spawnWave();
        }
    }
};
