#pragma once

#include "ansii_renderer.h"
#include <functional>

class Entity;

enum class ColliderShape { AABB, CIRCLE };

class ANSIICollider {
public:
    int id;
    int layer = 0;
    bool layerMask[8] = {true, true, true, true, true, true, true, true};
    Entity* owner = nullptr;

private:
    ANSIIRenderer* _renderer = nullptr;
    static inline int _nextId = 1;
    ColliderShape _shape = ColliderShape::AABB;
    float _radius = 0;

public:
    ANSIICollider(ANSIIRenderer* renderer, Entity* ownerEntity, int collisionLayer = 0)
        : _renderer(renderer), owner(ownerEntity), layer(collisionLayer) {
        id = _nextId++;
    }

    void setCircle(float radius) {
        _shape = ColliderShape::CIRCLE;
        _radius = radius;
    }

    ColliderShape getShape() const { return _shape; }
    float getRadius() const { return _radius; }

    float getX() const { return _renderer->x; }
    float getY() const { return _renderer->y; }
    int getWidth() const { return _renderer->width; }
    int getHeight() const { return _renderer->height; }

    // Center of the collider in world space
    float getCenterX() const { return _renderer->x + _renderer->width * 0.5f; }
    float getCenterY() const { return _renderer->y + _renderer->height * 0.5f; }

    // Iterate all cells this collider occupies, calling fn(cellX, cellY)
    template<typename Fn>
    void forEachCell(Fn&& fn) const {
        if (_shape == ColliderShape::CIRCLE) {
            float cx = getCenterX();
            float cy = getCenterY();
            int r = (int)_radius;
            int icx = (int)cx;
            int icy = (int)cy;
            float r2 = _radius * _radius;
            for (int dy = -r; dy <= r; dy++) {
                for (int dx = -r; dx <= r; dx++) {
                    if (dx * dx + dy * dy <= (int)r2) {
                        fn(icx + dx, icy + dy);
                    }
                }
            }
        } else {
            int cx = (int)getX();
            int cy = (int)getY();
            for (int i = 0; i < getHeight(); i++) {
                for (int j = 0; j < getWidth(); j++) {
                    fn(cx + j, cy + i);
                }
            }
        }
    }

    bool shouldCollideWith(int otherLayer) const {
        if (otherLayer < 0 || otherLayer >= 8) return false;
        return layerMask[otherLayer];
    }
};
