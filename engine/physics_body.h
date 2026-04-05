#pragma once

#include "ansii_renderer.h"
#include "tilemap.h"

class Entity;  // forward decl for TileCollision struct visibility

struct TileCollision {
    int tileX, tileY;
    int tileId;
    float normalX, normalY;  // 0,-1 = hit ceiling; 0,1 = hit ground; -1,0 = hit right wall; 1,0 = hit left wall
};

class PhysicsBody {
public:
    float velX = 0, velY = 0;
    float accelX = 0, accelY = 0;
    float dragX = 0, dragY = 0;
    float maxVelX = 0, maxVelY = 0;
    bool grounded = false;

    // Pre-resolution velocity — preserved for CollisionManager to use
    float preVelX = 0, preVelY = 0;

    // Angular state
    float angle = 0;
    float angularVel = 0;
    float angularAccel = 0;
    float angularDrag = 0;

private:
    ANSIIRenderer* _renderer = nullptr;
    TileMap* _tilemap = nullptr;

    // Tile collisions accumulated during this frame's resolve
    std::vector<TileCollision> _tileCollisions;

public:
    PhysicsBody(ANSIIRenderer* renderer) : _renderer(renderer) {}

    void setTileMap(TileMap* tm) { _tilemap = tm; }

    const std::vector<TileCollision>& getTileCollisions() const { return _tileCollisions; }

    void addForce(float fx, float fy) {
        velX += fx;
        velY += fy;
    }

    void addTorque(float t) {
        angularVel += t;
    }

    void setAccel(float ax, float ay) {
        accelX = ax;
        accelY = ay;
    }

    void update(float dt) {
        grounded = false;
        _tileCollisions.clear();

        // Snapshot pre-resolution velocity for entity-entity collision info
        preVelX = velX;
        preVelY = velY;

        // Angular update
        angularVel += angularAccel * dt;
        if (angularDrag > 0) angularVel *= (1.0f - angularDrag);
        angle += angularVel * dt;

        // Apply acceleration
        velX += accelX * dt;
        velY += accelY * dt;

        // Apply drag
        if (dragX > 0) velX *= (1.0f - dragX);
        if (dragY > 0) velY *= (1.0f - dragY);

        // Clamp to max velocity
        if (maxVelX > 0) {
            if (velX > maxVelX) velX = maxVelX;
            if (velX < -maxVelX) velX = -maxVelX;
        }
        if (maxVelY > 0) {
            if (velY > maxVelY) velY = maxVelY;
            if (velY < -maxVelY) velY = -maxVelY;
        }

        if (_tilemap) {
            resolveWithTilemap(dt);
        } else {
            _renderer->x += velX * dt;
            _renderer->y += velY * dt;
        }
    }

private:
    void resolveWithTilemap(float dt) {
        // Separate X and Y sweeps for proper collision direction

        // ---- X axis ----
        float newX = _renderer->x + velX * dt;
        if (velX != 0 && checkSolidRegion(newX, _renderer->y,
                                            _renderer->width, _renderer->height)) {
            float normalX = (velX > 0) ? -1.0f : 1.0f;
            // Record tile collisions on the leading edge
            recordTileHitsX(newX, _renderer->y, _renderer->width, _renderer->height,
                            normalX);

            if (velX > 0) {
                newX = (float)((int)(newX + _renderer->width) - _renderer->width);
                while (checkSolidRegion(newX, _renderer->y,
                                         _renderer->width, _renderer->height) && newX > _renderer->x) {
                    newX -= 1.0f;
                }
            } else {
                newX = (float)((int)newX + 1);
                while (checkSolidRegion(newX, _renderer->y,
                                         _renderer->width, _renderer->height) && newX < _renderer->x) {
                    newX += 1.0f;
                }
            }
            velX = 0;
        }
        _renderer->x = newX;

        // ---- Y axis ----
        float newY = _renderer->y + velY * dt;
        if (checkSolidRegion(_renderer->x, newY,
                              _renderer->width, _renderer->height)) {
            float normalY = (velY > 0) ? -1.0f : 1.0f;

            if (velY > 0) {
                // Moving down — land on ground
                recordTileHitsY(_renderer->x, newY, _renderer->width, _renderer->height,
                                normalY);
                newY = (float)((int)(newY + _renderer->height) - _renderer->height);
                while (checkSolidRegion(_renderer->x, newY,
                                         _renderer->width, _renderer->height) && newY > _renderer->y) {
                    newY -= 1.0f;
                }
                grounded = true;
            } else {
                // Moving up — bonk head
                recordTileHitsY(_renderer->x, newY, _renderer->width, _renderer->height,
                                normalY);
                newY = (float)((int)newY + 1);
                while (checkSolidRegion(_renderer->x, newY,
                                         _renderer->width, _renderer->height) && newY < _renderer->y) {
                    newY += 1.0f;
                }
            }
            velY = 0;
        }
        _renderer->y = newY;

        // Tile collisions are stored in _tileCollisions;
        // the engine loop dispatches them after physics runs.
    }

    // Check if any edge of the bounding box overlaps a solid tile
    bool checkSolidRegion(float px, float py, int w, int h) const {
        int left = (int)px;
        int right = (int)(px + w - 1);
        int top = (int)py;
        int bottom = (int)(py + h - 1);

        for (int tx = left; tx <= right; tx++) {
            if (_tilemap->isSolid(tx, top) || _tilemap->isSolid(tx, bottom)) return true;
        }
        for (int ty = top + 1; ty < bottom; ty++) {
            if (_tilemap->isSolid(left, ty) || _tilemap->isSolid(right, ty)) return true;
        }
        return false;
    }

    // Record solid tiles on the leading X edge
    void recordTileHitsX(float px, float py, int w, int h, float normalX) {
        int edgeX = (normalX < 0) ? (int)(px + w - 1) : (int)px;
        int top = (int)py;
        int bottom = (int)(py + h - 1);
        for (int ty = top; ty <= bottom; ty++) {
            if (_tilemap->isSolid(edgeX, ty)) {
                uint8_t id = _tilemap->getTile(edgeX, ty);
                _tileCollisions.push_back({edgeX, ty, id, normalX, 0});
            }
        }
    }

    // Record solid tiles on the leading Y edge
    void recordTileHitsY(float px, float py, int w, int h, float normalY) {
        int edgeY = (normalY < 0) ? (int)(py + h - 1) : (int)py;
        int left = (int)px;
        int right = (int)(px + w - 1);
        for (int tx = left; tx <= right; tx++) {
            if (_tilemap->isSolid(tx, edgeY)) {
                uint8_t id = _tilemap->getTile(tx, edgeY);
                _tileCollisions.push_back({tx, edgeY, id, 0, normalY});
            }
        }
    }
};
