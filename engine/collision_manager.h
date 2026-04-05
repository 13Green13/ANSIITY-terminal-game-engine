#pragma once

#include <vector>
#include <algorithm>
#include <utility>
#include <cmath>
#include "ansii_collider.h"
#include "entity.h"
#include "constants.h"

struct RayHit {
    bool hit = false;
    float x = 0, y = 0;    // world position of hit cell
    float distance = 0;     // from origin
    Entity* entity = nullptr;
    explicit operator bool() const { return hit; }
};

class CollisionManager {
    std::vector<ANSIICollider*> _colliders;
    std::vector<int> _grid;
    int _gridW = WIDTH;
    int _gridH = HEIGHT;
    std::vector<std::pair<int,int>> _dirtyCells;

    CollisionManager() : _grid(WIDTH * HEIGHT, 0) {}

public:
    static CollisionManager& getInstance() {
        static CollisionManager instance;
        return instance;
    }

    CollisionManager(const CollisionManager&) = delete;
    void operator=(const CollisionManager&) = delete;

    void setWorldSize(int w, int h) {
        _gridW = w;
        _gridH = h;
        _grid.assign((size_t)w * h, 0);
        _dirtyCells.clear();
    }

    void subscribe(ANSIICollider* collider) {
        _colliders.push_back(collider);
    }

    void unsubscribe(ANSIICollider* collider) {
        _colliders.erase(
            std::remove(_colliders.begin(), _colliders.end(), collider),
            _colliders.end()
        );
    }

    void resolve() {
        // Clear only dirty cells from last frame
        for (auto& [dx, dy] : _dirtyCells) {
            _grid[(size_t)dy * _gridW + dx] = 0;
        }
        _dirtyCells.clear();

        // Build collision matrix
        for (auto* col : _colliders) {
            col->forEachCell([&](int sx, int sy) {
                if (sx >= 0 && sx < _gridW && sy >= 0 && sy < _gridH) {
                    size_t idx = (size_t)sy * _gridW + sx;
                    int existing = _grid[idx];
                    if (existing != 0 && existing != col->id) {
                        ANSIICollider* other = findColliderById(existing);
                        if (other && col->shouldCollideWith(other->layer)
                            && other->shouldCollideWith(col->layer)) {
                            fireCollision(col, other);
                        }
                    }
                    _grid[idx] = col->id;
                    _dirtyCells.push_back({sx, sy});
                }
            });
        }
    }

    // Compute CollisionInfo and fire onCollision for both entities
    void fireCollision(ANSIICollider* colA, ANSIICollider* colB) {
        Entity* a = colA->owner;
        Entity* b = colB->owner;

        // Compute collision normal from A's perspective (points from B toward A)
        float dx = (colA->getX() + colA->getWidth() * 0.5f)
                  - (colB->getX() + colB->getWidth() * 0.5f);
        float dy = (colA->getY() + colA->getHeight() * 0.5f)
                  - (colB->getY() + colB->getHeight() * 0.5f);

        float nx, ny;
        if (std::abs(dx) > std::abs(dy)) {
            nx = (dx > 0) ? 1.0f : -1.0f;
            ny = 0;
        } else {
            nx = 0;
            ny = (dy > 0) ? 1.0f : -1.0f;
        }

        // Get pre-resolution velocities
        float aPreVX = 0, aPreVY = 0, bPreVX = 0, bPreVY = 0;
        if (a->physics) { aPreVX = a->physics->preVelX; aPreVY = a->physics->preVelY; }
        if (b->physics) { bPreVX = b->physics->preVelX; bPreVY = b->physics->preVelY; }

        CollisionInfo infoA = { nx, ny, aPreVX, aPreVY, bPreVX, bPreVY };
        CollisionInfo infoB = { -nx, -ny, bPreVX, bPreVY, aPreVX, aPreVY };

        a->onCollision(b, infoA);
        b->onCollision(a, infoB);
    }

    // DDA raycast against the entity collision grid
    // Walks cells along a direction, returns first hit entity
    RayHit raycast(float originX, float originY, float dirX, float dirY,
                   float maxDist = 500.0f, int layerMask = -1) {
        RayHit result;
        float len = std::sqrt(dirX * dirX + dirY * dirY);
        if (len < 1e-8f) return result;
        dirX /= len;
        dirY /= len;

        int mapX = (int)originX;
        int mapY = (int)originY;

        float deltaDistX = (dirX == 0) ? 1e30f : std::abs(1.0f / dirX);
        float deltaDistY = (dirY == 0) ? 1e30f : std::abs(1.0f / dirY);

        int stepX, stepY;
        float sideDistX, sideDistY;

        if (dirX < 0) {
            stepX = -1;
            sideDistX = (originX - mapX) * deltaDistX;
        } else {
            stepX = 1;
            sideDistX = (mapX + 1.0f - originX) * deltaDistX;
        }
        if (dirY < 0) {
            stepY = -1;
            sideDistY = (originY - mapY) * deltaDistY;
        } else {
            stepY = 1;
            sideDistY = (mapY + 1.0f - originY) * deltaDistY;
        }

        float dist = 0;
        while (dist < maxDist) {
            if (sideDistX < sideDistY) {
                mapX += stepX;
                dist = sideDistX;
                sideDistX += deltaDistX;
            } else {
                mapY += stepY;
                dist = sideDistY;
                sideDistY += deltaDistY;
            }

            if (mapX < 0 || mapX >= _gridW || mapY < 0 || mapY >= _gridH) break;

            int id = _grid[(size_t)mapY * _gridW + mapX];
            if (id != 0) {
                ANSIICollider* col = findColliderById(id);
                if (col && (layerMask == -1 || (layerMask & (1 << col->layer)))) {
                    result.hit = true;
                    result.x = (float)mapX;
                    result.y = (float)mapY;
                    result.distance = dist;
                    result.entity = col->owner;
                    return result;
                }
            }
        }
        return result;
    }

    // Raycast using degrees: 0=down, 90=right, 180=up, 270=left
    RayHit raycastAngle(float originX, float originY, float angleDeg,
                        float maxDist = 500.0f, int layerMask = -1) {
        constexpr float DEG2RAD = 3.14159265f / 180.0f;
        float rad = angleDeg * DEG2RAD;
        float dirX = std::sin(rad);   // 0°→0, 90°→1, 180°→0, 270°→-1
        float dirY = std::cos(rad);   // 0°→1, 90°→0, 180°→-1, 270°→0
        return raycast(originX, originY, dirX, dirY, maxDist, layerMask);
    }

private:
    ANSIICollider* findColliderById(int id) {
        for (auto* col : _colliders) {
            if (col->id == id) return col;
        }
        return nullptr;
    }
};
