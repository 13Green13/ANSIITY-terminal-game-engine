#pragma once

#include "../../engine/engine.h"
#include "../../engine/scene_loader.h"
#include <cstdlib>
#include <cmath>

class BatEntity : public Entity {
    float _flySpeed = 35.0f;
    float _dirX, _dirY;
    float _changeTimer;
    float _bobPhase;

public:
    BatEntity(float x, float y, const Props&) {
        tag = "bat";

        renderer = new ANSIIRenderer(x, y);
        renderer->loadFromFile("sprites/bat.ansii");
        renderer->zOrder = 20;

        collider = new ANSIICollider(renderer, this, 1);
        collider->layerMask[1] = false;  // don't collide with other bats

        _dirX = ((rand() % 2) ? 1.0f : -1.0f);
        _dirY = 0;
        _changeTimer = 1.0f + (float)(rand() % 100) / 50.0f;
        _bobPhase = (float)(rand() % 628) / 100.0f;
    }

    void update(float dt) override {
        _bobPhase += dt * 4.0f;
        _changeTimer -= dt;

        // Randomly change direction
        if (_changeTimer <= 0) {
            _changeTimer = 1.0f + (float)(rand() % 200) / 100.0f;
            _dirX = ((float)(rand() % 200) / 100.0f - 1.0f);
            _dirY = ((float)(rand() % 200) / 100.0f - 1.0f) * 0.3f;
            float len = std::sqrt(_dirX * _dirX + _dirY * _dirY);
            if (len > 0.01f) { _dirX /= len; _dirY /= len; }
        }

        // Check walls using tilemap
        TileMap* tm = SceneLoader::getInstance().getTileMap();
        if (tm) {
            int checkX = (int)(renderer->x + _dirX * 3.0f);
            int checkY = (int)(renderer->y + 1);
            if (tm->isSolid(checkX, checkY)) {
                _dirX = -_dirX;
            }
            int checkY2 = (int)(renderer->y + _dirY * 3.0f);
            if (tm->isSolid((int)renderer->x + 1, checkY2)) {
                _dirY = -_dirY;
            }
        }

        renderer->x += _dirX * _flySpeed * dt;
        renderer->y += _dirY * _flySpeed * dt + std::sin(_bobPhase) * 0.15f;
    }
};
