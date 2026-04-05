#pragma once

#include "../../engine/engine.h"
#include "../../engine/scene_loader.h"

class BombEntity : public Entity {
    float _fuseTimer = 2.0f;
    bool _stuck = false;
    int _explosionRadius = 4;

public:
    BombEntity(float x, float y, float vx, float vy) {
        tag = "bomb";

        renderer = new ANSIIRenderer(x, y);
        renderer->loadFromFile("sprites/bomb.ansii");
        renderer->zOrder = 25;

        physics = new PhysicsBody(renderer);
        physics->velX = vx;
        physics->velY = vy;
        physics->setAccel(0, 200);
        physics->dragX = 0.05f;
    }

    void update(float dt) override {
        _fuseTimer -= dt;

        // Blink faster as fuse runs out
        if (_fuseTimer < 0.8f) {
            int blink = (int)(_fuseTimer * 10) % 2;
            renderer->zOrder = (blink == 0) ? 25 : -1;
        }

        if (_fuseTimer <= 0) {
            explode();
            destroy();
        }
    }

    void onTileCollision(const TileCollision& tc) override {
        if (!_stuck) {
            _stuck = true;
            physics->velX = 0;
            physics->velY = 0;
            physics->setAccel(0, 0);
        }
    }

private:
    void explode() {
        auto* tilemap = SceneLoader::getInstance().getTileMap();
        if (!tilemap) return;

        int cx = (int)(renderer->x + 0.5f);
        int cy = (int)(renderer->y + 0.5f);
        int r = _explosionRadius;

        for (int dy = -r; dy <= r; dy++) {
            for (int dx = -r; dx <= r; dx++) {
                if (dx * dx + dy * dy <= r * r) {
                    int tx = cx + dx;
                    int ty = cy + dy;
                    if (tilemap->isSolid(tx, ty)) {
                        tilemap->setTile(tx, ty, 0);
                    }
                }
            }
        }
    }
};
