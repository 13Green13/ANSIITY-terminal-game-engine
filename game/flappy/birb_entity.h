#pragma once

#include "../../engine/engine.h"

constexpr float JUMP_VELOCITY = -150.0f;
constexpr float BIRB_GRAVITY = 900.0f;

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
            _velocity = JUMP_VELOCITY;
        }

        _velocity += BIRB_GRAVITY * dt;
        renderer->y += _velocity * dt;

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
