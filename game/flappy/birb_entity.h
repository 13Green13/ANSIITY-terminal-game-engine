#pragma once

#include "../../engine/engine.h"

constexpr int JUMPHEIGHT = 5;
constexpr float BIRB_GRAVITY = 0.3f;

class BirbEntity : public Entity {
    float _velocity = 0;

public:
    BirbEntity(float x, float y) {
        renderer = new ANSIIRenderer(x, y);
        renderer->zOrder = 20;
        renderer->loadFromFile("sprites/birb.ansii");
        collider = new ANSIICollider(renderer, this, 0);
        InputManager::getInstance().watchKey(VK_SPACE);
    }

    void update(float dt) override {
        // Input
        auto& input = InputManager::getInstance();
        if (input.isKeyPressed(VK_SPACE)) {
            _velocity -= JUMPHEIGHT;
        }

        float scale = dt * TARGET_FPS;
        _velocity += BIRB_GRAVITY * scale;
        renderer->y += _velocity;

        if (renderer->y >= HEIGHT - renderer->height) {
            renderer->y = HEIGHT - renderer->height;
            _velocity = 0;
        }
        if (renderer->y < 0) {
            renderer->y = 0;
            _velocity = 0;
        }
    }

    void onCollision(Entity* other, const CollisionInfo& info) override {
        // Game over — for now just exit
        exit(0);
    }
};
