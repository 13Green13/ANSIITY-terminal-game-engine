#pragma once

#include <vector>
#include <algorithm>
#include "entity.h"

class CollisionManager;
class RenderManager;

class EntityManager {
    std::vector<Entity*> _entities;

    EntityManager() = default;

public:
    static EntityManager& getInstance() {
        static EntityManager instance;
        return instance;
    }

    EntityManager(const EntityManager&) = delete;
    void operator=(const EntityManager&) = delete;

    void subscribe(Entity* entity) {
        _entities.push_back(entity);
    }

    void update(float dt) {
        for (auto* e : _entities) {
            if (e->alive) {
                e->update(dt);
            }
        }
        sweep();
    }

    void sweep();  // defined in engine.h after CollisionManager/RenderManager

    const std::vector<Entity*>& getEntities() const { return _entities; }
};
