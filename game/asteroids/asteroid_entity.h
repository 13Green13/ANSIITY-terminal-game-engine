#pragma once

#include "../../engine/engine.h"
#include "../../engine/constants.h"
#include "game_state.h"
#include <cstdlib>
#include <cmath>

enum class AsteroidSize { LARGE, MEDIUM, SMALL };

class AsteroidEntity : public Entity {
    float _velX, _velY;
    AsteroidSize _size;

    static float randFloat(float lo, float hi) {
        return lo + (float)rand() / RAND_MAX * (hi - lo);
    }

public:
    AsteroidEntity(float x, float y, AsteroidSize size, float vx, float vy)
        : _velX(vx), _velY(vy), _size(size)
    {
        tag = "asteroid";

        renderer = new ANSIIRenderer(x, y);
        renderer->zOrder = 15;

        switch (size) {
            case AsteroidSize::LARGE:
                renderer->loadFromFile("sprites/asteroid_large.ansii");
                break;
            case AsteroidSize::MEDIUM:
                renderer->loadFromFile("sprites/asteroid_medium.ansii");
                break;
            case AsteroidSize::SMALL:
                renderer->loadFromFile("sprites/asteroid_small.ansii");
                break;
        }

        collider = new ANSIICollider(renderer, this, 2);
        collider->layerMask[2] = false;  // don't collide with other asteroids
    }

    AsteroidSize getSize() const { return _size; }

    void update(float dt) override {
        renderer->x += _velX * dt;
        renderer->y += _velY * dt;

        // Screen wrap
        if (renderer->x + renderer->width < 0) renderer->x = (float)WIDTH;
        if (renderer->x > WIDTH) renderer->x = (float)(-renderer->width);
        if (renderer->y + renderer->height < 0) renderer->y = (float)HEIGHT;
        if (renderer->y > HEIGHT) renderer->y = (float)(-renderer->height);
    }

    void onCollision(Entity* other, const CollisionInfo& info) override {
        if (other->tag == "bullet") {
            // Score
            switch (_size) {
                case AsteroidSize::LARGE:  GameState::score += 20; break;
                case AsteroidSize::MEDIUM: GameState::score += 50; break;
                case AsteroidSize::SMALL:  GameState::score += 100; break;
            }

            // Split into 2 smaller asteroids
            if (_size != AsteroidSize::SMALL) {
                AsteroidSize nextSize = (_size == AsteroidSize::LARGE)
                    ? AsteroidSize::MEDIUM : AsteroidSize::SMALL;

                float speedMult = (_size == AsteroidSize::LARGE) ? 1.5f : 2.0f;

                for (int i = 0; i < 2; i++) {
                    float angle = randFloat(0, 6.2832f);
                    float speed = std::sqrt(_velX * _velX + _velY * _velY) * speedMult;
                    float nvx = std::cos(angle) * speed;
                    float nvy = std::sin(angle) * speed;

                    auto* child = new AsteroidEntity(
                        renderer->x, renderer->y, nextSize, nvx, nvy);
                    child->init();
                }
            }

            destroy();
        }
    }
};
