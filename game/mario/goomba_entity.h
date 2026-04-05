#pragma once

#include "../../engine/engine.h"
#include "../../engine/scene_loader.h"

class GoombaEntity : public Entity {
    float _patrolSpeed = 30.0f;
    float _dir;

public:
    GoombaEntity(float x, float y, const Props& props) {
        tag = "goomba";

        renderer = new ANSIIRenderer(x, y);
        renderer->loadFromFile("sprites/goomba.ansii");
        renderer->zOrder = 20;

        collider = new ANSIICollider(renderer, this, 1);
        collider->layerMask[1] = false;  // don't collide with other goombas

        physics = new PhysicsBody(renderer);
        physics->setAccel(0, 600);  // gravity

        _dir = (props.count("dir") && props.at("dir") == "right") ? 1.0f : -1.0f;
    }

    void update(float dt) override {
        if (physics->grounded) {
            // Ledge detection: check if ground exists ahead
            TileMap* tm = SceneLoader::getInstance().getTileMap();
            if (tm) {
                int checkX = (int)(renderer->x + (_dir > 0 ? renderer->width + 1 : -1));
                int footY = (int)(renderer->y + renderer->height + 1);
                if (!tm->isSolid(checkX, footY)) {
                    _dir = -_dir;  // ledge ahead, turn around
                }
            }

            // Wall detection: physics zeroed velX means we hit a wall
            if (physics->velX == 0) {
                _dir = -_dir;
            }
        }
        physics->velX = _dir * _patrolSpeed;

        // Fall death
        if (renderer->y > 65) {
            destroy();
        }
    }
};
