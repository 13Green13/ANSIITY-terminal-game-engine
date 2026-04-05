#pragma once

#include "../../engine/engine.h"
#include "../../engine/input_manager.h"
#include "../../engine/scene_loader.h"
#include "game_state.h"
#include "bomb_entity.h"

class MinerEntity : public Entity {
    float _moveSpeed = 80.0f;
    float _thrustPower = -600.0f;  // jetpack thrust (fights gravity)
    float _invTimer = 0;
    int _facingDir = 1;  // 1 = right, -1 = left
    float _bombCooldown = 0;

public:
    MinerEntity(float x, float y, const Props&) {
        tag = "miner";

        renderer = new ANSIIRenderer(x, y);
        renderer->loadFromFile("sprites/miner.ansii");
        renderer->zOrder = 30;

        collider = new ANSIICollider(renderer, this, 0);

        physics = new PhysicsBody(renderer);
        physics->setAccel(0, 200);    // lighter gravity for floaty feel
        physics->maxVelY = 120;
        physics->dragX = 0.15f;
        physics->dragY = 0.05f;       // slight air drag for smooth deceleration

        auto& input = InputManager::getInstance();
        input.watchKey(VK_LEFT);
        input.watchKey(VK_RIGHT);
        input.watchKey(VK_UP);
        input.watchKey(VK_DOWN);
        input.watchKey(VK_SPACE);
        input.watchKey('Z');
        input.watchKey('X');
    }

    void update(float dt) override {
        auto& input = InputManager::getInstance();

        // Horizontal movement
        if (input.isKeyHeld(VK_LEFT)) {
            physics->velX = -_moveSpeed;
            _facingDir = -1;
        } else if (input.isKeyHeld(VK_RIGHT)) {
            physics->velX = _moveSpeed;
            _facingDir = 1;
        }

        // Jetpack: hold Space/Z/Up to thrust upward
        bool thrustKey = input.isKeyHeld(VK_SPACE) || input.isKeyHeld('Z') || input.isKeyHeld(VK_UP);
        if (thrustKey) {
            physics->addForce(0, _thrustPower * dt);
        }

        // Down key: descend faster
        if (input.isKeyHeld(VK_DOWN)) {
            physics->addForce(0, 300.0f * dt);
        }

        // Invincibility timer
        if (_invTimer > 0) {
            _invTimer -= dt;
        }

        // Bomb cooldown
        if (_bombCooldown > 0) {
            _bombCooldown -= dt;
        }

        // Throw bomb on X press
        if (input.isKeyPressed('X') && _bombCooldown <= 0) {
            _bombCooldown = 0.5f;
            float bx = renderer->x + (_facingDir > 0 ? renderer->width : -1);
            float by = renderer->y + 1;
            float bvx = _facingDir * 120.0f;
            float bvy = -40.0f;
            auto* bomb = new BombEntity(bx, by, bvx, bvy);
            auto* tilemap = SceneLoader::getInstance().getTileMap();
            if (bomb->physics && tilemap) {
                bomb->physics->setTileMap(tilemap);
            }
            bomb->init();
        }
    }

    void onCollision(Entity* other, const CollisionInfo& info) override {
        if (other->tag == "gem") {
            other->destroy();
            CavernState::gems++;
        } else if (other->tag == "bat") {
            if (_invTimer <= 0) {
                // Knockback on contact
                _invTimer = 1.0f;
                float knockX = (info.normalX > 0) ? 50.0f : -50.0f;
                float knockY = (info.normalY > 0) ? 50.0f : -50.0f;
                physics->velX = knockX;
                physics->velY = knockY;
            }
        } else if (other->tag == "exit") {
            if (CavernState::gems >= CavernState::totalGems) {
                CavernState::won = true;
                Engine::getInstance().stop();
            }
        }
    }
};
