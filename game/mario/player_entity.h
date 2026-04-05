#pragma once

#include "../../engine/engine.h"
#include "../../engine/input_manager.h"
#include "../../engine/scene_loader.h"
#include "powerup_entity.h"

class PlayerEntity : public Entity {
    float _moveSpeed = 80.0f;
    float _jumpForce = -110.0f;
    bool _jumpHeld = false;

public:
    PlayerEntity(float x, float y, const Props&) {
        tag = "player";

        renderer = new ANSIIRenderer(x, y);
        renderer->loadFromFile("sprites/player.ansii");
        renderer->zOrder = 30;

        collider = new ANSIICollider(renderer, this, 0);

        physics = new PhysicsBody(renderer);
        physics->setAccel(0, 400);   // gravity
        physics->maxVelY = 250;
        physics->dragX = 0.25f;

        auto& input = InputManager::getInstance();
        input.watchKey(VK_LEFT);
        input.watchKey(VK_RIGHT);
        input.watchKey(VK_SPACE);
    }

    void update(float dt) override {
        auto& input = InputManager::getInstance();

        if (input.isKeyHeld(VK_LEFT)) {
            physics->velX = -_moveSpeed;
        } else if (input.isKeyHeld(VK_RIGHT)) {
            physics->velX = _moveSpeed;
        }

        if (input.isKeyHeld(VK_SPACE)) {
            if (physics->grounded && !_jumpHeld) {
                physics->addForce(0, _jumpForce);
                _jumpHeld = true;
            }
        } else {
            _jumpHeld = false;
        }

        if (renderer->y > 65) {
            Engine::getInstance().stop();
        }
    }

    void onTileCollision(const TileCollision& tc) override {
        // Hit a ? block from below
        if (tc.normalY > 0 && tc.tileId == 3) {
            TileMap* tm = SceneLoader::getInstance().getTileMap();
            if (tm) {
                tm->setTile(tc.tileX, tc.tileY, 1);  // ? → brick
                auto* pu = new PowerupEntity((float)tc.tileX - 1, (float)tc.tileY - 4, {});
                pu->init();
            }
        }
    }

    void onCollision(Entity* other, const CollisionInfo& info) override {
        if (other->tag == "goomba") {
            // Stomp: player's feet are in the top half of the goomba
            float playerBottom = renderer->y + renderer->height;
            float goombaMid = other->renderer->y + other->renderer->height * 0.5f;
            if (playerBottom <= goombaMid) {
                other->destroy();
                physics->velY = 0;
                physics->addForce(0, _jumpForce * 0.7f);
            } else {
                Engine::getInstance().stop();
            }
        } else if (other->tag == "powerup") {
            other->destroy();
            _moveSpeed = 110.0f;
            _jumpForce = -140.0f;
        } else if (other->tag == "finish") {
            Engine::getInstance().stop();
        }
    }
};
