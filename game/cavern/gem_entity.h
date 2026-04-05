#pragma once

#include "../../engine/engine.h"
#include "game_state.h"
#include <cmath>

class GemEntity : public Entity {
    float _baseY;
    float _bobPhase;

public:
    GemEntity(float x, float y, const std::string& sprite) {
        tag = "gem";

        renderer = new ANSIIRenderer(x, y);
        renderer->loadFromFile(sprite);
        renderer->zOrder = 22;

        collider = new ANSIICollider(renderer, this, 2);
        collider->layerMask[1] = false;  // don't collide with bats
        collider->layerMask[2] = false;  // don't collide with other gems

        _baseY = y;
        _bobPhase = (float)((int)(x * 100) % 628) / 100.0f;

        CavernState::totalGems++;
    }

    void update(float dt) override {
        _bobPhase += dt * 2.5f;
        renderer->y = _baseY + std::sin(_bobPhase) * 0.8f;
    }
};

// Factory wrappers for the three gem colors
class GemCyanEntity : public GemEntity {
public:
    GemCyanEntity(float x, float y, const Props&) : GemEntity(x, y, "sprites/gem_cyan.ansii") {}
};

class GemGreenEntity : public GemEntity {
public:
    GemGreenEntity(float x, float y, const Props&) : GemEntity(x, y, "sprites/gem_green.ansii") {}
};

class GemRedEntity : public GemEntity {
public:
    GemRedEntity(float x, float y, const Props&) : GemEntity(x, y, "sprites/gem_red.ansii") {}
};
