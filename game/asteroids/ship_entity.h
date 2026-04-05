#pragma once

#include "../../engine/engine.h"
#include "../../engine/input_manager.h"
#include "../../engine/constants.h"
#include "bullet_entity.h"
#include <cmath>

class ShipEntity : public Entity {
    int _dir = 0;             // 0-7, clockwise from Up
    float _velX = 0, _velY = 0;
    float _thrustPower = 200.0f;
    float _maxSpeed = 120.0f;
    float _shootCooldown = 0;
    float _rotTimer = 0;
    float _bulletSpeed = 200.0f;

    // Direction vectors for 8 directions (dx, dy)
    static constexpr float _dirX[8] = { 0.0f,  0.707f,  1.0f,  0.707f,  0.0f, -0.707f, -1.0f, -0.707f };
    static constexpr float _dirY[8] = {-1.0f, -0.707f,  0.0f,  0.707f,  1.0f,  0.707f,  0.0f, -0.707f };

    // 8 ship sprites, each 5x5 = 25 cells
    // T=transparent, C=bright cyan (body), N=bright white (nose)
    static constexpr int T = 0;
    static constexpr int C = 96;
    static constexpr int N = 97;

    void updateSprite() {
        // Each sprite is row-major 5 wide x 5 tall
        // Clear triangular silhouette with a bright nose tip
        static const SpriteCell sprites[8][25] = {
            // dir 0 (Up):
            //  . . ^ . .
            //  . / | \ .
            //  / . | . bksl
            //  | . . . |
            //  . . . . .
            {
                {'.', T}, {'.', T}, {'^', N}, {'.', T}, {'.', T},
                {'.', T}, {'/', C}, {'|', C}, {'\\', C}, {'.', T},
                {'/', C}, {'.', T}, {'|', C}, {'.', T}, {'\\', C},
                {'|', C}, {'.', T}, {'.', T}, {'.', T}, {'|', C},
                {'.', T}, {'.', T}, {'.', T}, {'.', T}, {'.', T},
            },
            // dir 1 (UpRight):
            //  . . . / >
            //  . . / . .
            //  . / . . .
            //  / . . . .
            //  . . . . .
            {
                {'.', T}, {'.', T}, {'.', T}, {'/', C}, {'>', N},
                {'.', T}, {'.', T}, {'/', C}, {'.', T}, {'/', C},
                {'.', T}, {'/', C}, {'.', T}, {'.', T}, {'.', T},
                {'/', C}, {'.', T}, {'.', T}, {'.', T}, {'.', T},
                {'\\', C}, {'.', T}, {'.', T}, {'.', T}, {'.', T},
            },
            // dir 2 (Right):
            //  . . . . .
            //  | . . / .
            //  - - - - >
            //  | . . \ .
            //  . . . . .
            {
                {'.', T}, {'.', T}, {'.', T}, {'.', T}, {'.', T},
                {'|', C}, {'.', T}, {'.', T}, {'/', C}, {'.', T},
                {'-', C}, {'-', C}, {'-', C}, {'-', C}, {'>', N},
                {'|', C}, {'.', T}, {'.', T}, {'\\', C}, {'.', T},
                {'.', T}, {'.', T}, {'.', T}, {'.', T}, {'.', T},
            },
            // dir 3 (DownRight):
            //  \ . . . .
            //  . \ . . .
            //  . . \ . .
            //  . . . \ .
            //  . . . \ >
            {
                {'\\', C}, {'.', T}, {'.', T}, {'.', T}, {'.', T},
                {'.', T}, {'\\', C}, {'.', T}, {'.', T}, {'.', T},
                {'.', T}, {'.', T}, {'\\', C}, {'.', T}, {'.', T},
                {'.', T}, {'.', T}, {'.', T}, {'\\', C}, {'\\', C},
                {'.', T}, {'.', T}, {'.', T}, {'\\', C}, {'>', N},
            },
            // dir 4 (Down):
            //  . . . . .
            //  | . . . |
            //  \ . | . /
            //  . \ | / .
            //  . . v . .
            {
                {'.', T}, {'.', T}, {'.', T}, {'.', T}, {'.', T},
                {'|', C}, {'.', T}, {'.', T}, {'.', T}, {'|', C},
                {'\\', C}, {'.', T}, {'|', C}, {'.', T}, {'/', C},
                {'.', T}, {'\\', C}, {'|', C}, {'/', C}, {'.', T},
                {'.', T}, {'.', T}, {'v', N}, {'.', T}, {'.', T},
            },
            // dir 5 (DownLeft):
            //  . . . . /
            //  . . . / .
            //  . . / . .
            //  . / . . .
            //  < / . . .
            {
                {'.', T}, {'.', T}, {'.', T}, {'.', T}, {'/', C},
                {'.', T}, {'.', T}, {'.', T}, {'/', C}, {'.', T},
                {'.', T}, {'.', T}, {'/', C}, {'.', T}, {'.', T},
                {'/', C}, {'/', C}, {'.', T}, {'.', T}, {'.', T},
                {'<', N}, {'/', C}, {'.', T}, {'.', T}, {'.', T},
            },
            // dir 6 (Left):
            //  . . . . .
            //  . \ . . |
            //  < - - - -
            //  . / . . |
            //  . . . . .
            {
                {'.', T}, {'.', T}, {'.', T}, {'.', T}, {'.', T},
                {'.', T}, {'\\', C}, {'.', T}, {'.', T}, {'|', C},
                {'<', N}, {'-', C}, {'-', C}, {'-', C}, {'-', C},
                {'.', T}, {'/', C}, {'.', T}, {'.', T}, {'|', C},
                {'.', T}, {'.', T}, {'.', T}, {'.', T}, {'.', T},
            },
            // dir 7 (UpLeft):
            //  < \ . . .
            //  . \ . . .
            //  . . \ . .
            //  . . . \ .
            //  . . . . bksl
            {
                {'<', N}, {'\\', C}, {'.', T}, {'.', T}, {'.', T},
                {'\\', C}, {'\\', C}, {'.', T}, {'.', T}, {'.', T},
                {'.', T}, {'.', T}, {'\\', C}, {'.', T}, {'.', T},
                {'.', T}, {'.', T}, {'.', T}, {'\\', C}, {'.', T},
                {'.', T}, {'.', T}, {'.', T}, {'.', T}, {'\\', C},
            },
        };

        std::vector<SpriteCell> cells(sprites[_dir], sprites[_dir] + 25);
        renderer->loadFromData(5, 5, cells);
    }

public:
    ShipEntity(float x, float y, const Props&) {
        tag = "ship";

        renderer = new ANSIIRenderer(x, y);
        renderer->zOrder = 30;

        collider = new ANSIICollider(renderer, this, 0);
        collider->layerMask[1] = false;  // don't collide with bullets

        auto& input = InputManager::getInstance();
        input.watchKey(VK_LEFT);
        input.watchKey(VK_RIGHT);
        input.watchKey(VK_UP);
        input.watchKey(VK_SPACE);

        updateSprite();
    }

    void update(float dt) override {
        auto& input = InputManager::getInstance();

        // Rotation with auto-repeat
        _rotTimer -= dt;
        if (_rotTimer <= 0) {
            if (input.isKeyHeld(VK_LEFT)) {
                _dir = (_dir + 7) % 8;
                _rotTimer = 0.1f;
                updateSprite();
            }
            if (input.isKeyHeld(VK_RIGHT)) {
                _dir = (_dir + 1) % 8;
                _rotTimer = 0.1f;
                updateSprite();
            }
        }

        // Thrust
        if (input.isKeyHeld(VK_UP)) {
            _velX += _dirX[_dir] * _thrustPower * dt;
            _velY += _dirY[_dir] * _thrustPower * dt;
        }

        // Drag (space feel — slow deceleration)
        _velX *= 0.99f;
        _velY *= 0.99f;

        // Speed cap
        float speed = std::sqrt(_velX * _velX + _velY * _velY);
        if (speed > _maxSpeed) {
            _velX = _velX / speed * _maxSpeed;
            _velY = _velY / speed * _maxSpeed;
        }

        // Move
        renderer->x += _velX * dt;
        renderer->y += _velY * dt;

        // Screen wrap
        if (renderer->x + renderer->width < 0) renderer->x = (float)WIDTH;
        if (renderer->x > WIDTH) renderer->x = (float)(-renderer->width);
        if (renderer->y + renderer->height < 0) renderer->y = (float)HEIGHT;
        if (renderer->y > HEIGHT) renderer->y = (float)(-renderer->height);

        // Shoot
        _shootCooldown -= dt;
        if (input.isKeyHeld(VK_SPACE) && _shootCooldown <= 0) {
            _shootCooldown = 0.2f;

            float bx = renderer->x + 2.0f;  // center of 5x5
            float by = renderer->y + 2.0f;
            float bvx = _dirX[_dir] * _bulletSpeed + _velX * 0.5f;
            float bvy = _dirY[_dir] * _bulletSpeed + _velY * 0.5f;

            auto* bullet = new BulletEntity(bx, by, bvx, bvy);
            bullet->init();
        }
    }

    void onCollision(Entity* other, const CollisionInfo& info) override {
        if (other->tag == "asteroid") {
            Engine::getInstance().stop();
        }
    }
};
