#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <functional>
#include <vector>
#include "entity.h"
#include "entity_manager.h"
#include "camera.h"
#include "tilemap.h"
#include "tilemap_renderer.h"
#include "render_manager.h"
#include "collision_manager.h"

using Props = std::unordered_map<std::string, std::string>;
using EntityFactory = std::function<Entity*(float, float, const Props&)>;

class SceneLoader {
    std::unordered_map<std::string, EntityFactory> _factories;

    // Owned by the scene — cleaned up on clear/reload
    TilePalette* _palette = nullptr;
    TileMap* _tilemap = nullptr;
    Camera* _camera = nullptr;
    Entity* _tilemapEntity = nullptr;

    // Tag → entity mapping for camera_follow etc.
    std::unordered_map<std::string, Entity*> _taggedEntities;

    SceneLoader() = default;

public:
    static SceneLoader& getInstance() {
        static SceneLoader instance;
        return instance;
    }

    SceneLoader(const SceneLoader&) = delete;
    void operator=(const SceneLoader&) = delete;

    void registerFactory(const std::string& type, EntityFactory factory) {
        _factories[type] = std::move(factory);
    }

    TileMap* getTileMap() const { return _tilemap; }
    Camera* getCamera() const { return _camera; }

    Entity* findByTag(const std::string& tag) {
        auto it = _taggedEntities.find(tag);
        return (it != _taggedEntities.end()) ? it->second : nullptr;
    }

    bool load(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) return false;

        // Deferred settings (need entities spawned first for camera_follow)
        std::string cameraFollowTag;
        float cameraSmooth = 0;
        float cameraDZW = 0, cameraDZH = 0;
        float cameraStartX = 0, cameraStartY = 0;
        bool hasCamStart = false;
        float camBoundsMinX = 0, camBoundsMinY = 0;
        float camBoundsMaxX = 0, camBoundsMaxY = 0;
        bool hasCamBounds = false;

        std::string palettePath, tilemapPath;
        struct SpawnEntry {
            std::string type;
            float x, y;
            Props props;
        };
        std::vector<SpawnEntry> spawns;

        // Parse file
        std::string line;
        while (std::getline(file, line)) {
            // Strip comments
            size_t commentPos = line.find('#');
            if (commentPos != std::string::npos) line = line.substr(0, commentPos);

            std::istringstream iss(line);
            std::string keyword;
            if (!(iss >> keyword)) continue;

            if (keyword == "tilemap") {
                iss >> tilemapPath;
            } else if (keyword == "palette") {
                iss >> palettePath;
            } else if (keyword == "camera_follow") {
                iss >> cameraFollowTag;
            } else if (keyword == "camera_smooth") {
                iss >> cameraSmooth;
            } else if (keyword == "camera_deadzone") {
                iss >> cameraDZW >> cameraDZH;
            } else if (keyword == "camera_start") {
                iss >> cameraStartX >> cameraStartY;
                hasCamStart = true;
            } else if (keyword == "camera_bounds") {
                iss >> camBoundsMinX >> camBoundsMinY >> camBoundsMaxX >> camBoundsMaxY;
                hasCamBounds = true;
            } else if (keyword == "spawn") {
                SpawnEntry entry;
                if (!(iss >> entry.type >> entry.x >> entry.y)) continue;
                // Parse optional key=value props
                std::string token;
                while (iss >> token) {
                    size_t eq = token.find('=');
                    if (eq != std::string::npos) {
                        entry.props[token.substr(0, eq)] = token.substr(eq + 1);
                    }
                }
                spawns.push_back(std::move(entry));
            }
        }

        // Setup tilemap
        if (!palettePath.empty()) {
            _palette = new TilePalette();
            _palette->loadFromFile(palettePath);
        }
        if (!tilemapPath.empty() && _palette) {
            _tilemap = new TileMap();
            _tilemap->setPalette(_palette);
            _tilemap->loadFromFile(tilemapPath);

            // Create tilemap entity
            _tilemapEntity = new Entity();
            _tilemapEntity->renderer = new TileMapRenderer(_tilemap);
            _tilemapEntity->renderer->zOrder = 5;
            _tilemapEntity->init();
        }

        // Set collision world size to match tilemap
        if (_tilemap) {
            CollisionManager::getInstance().setWorldSize(
                _tilemap->getMapWidth(), _tilemap->getMapHeight());
        }

        // Setup camera
        _camera = new Camera();
        if (hasCamStart) {
            _camera->x = cameraStartX;
            _camera->y = cameraStartY;
        }
        _camera->smoothing = cameraSmooth;
        _camera->deadZoneW = cameraDZW;
        _camera->deadZoneH = cameraDZH;
        RenderManager::getInstance().setActiveCamera(_camera);

        // Spawn entities
        for (auto& s : spawns) {
            auto it = _factories.find(s.type);
            if (it == _factories.end()) continue;

            Entity* e = it->second(s.x, s.y, s.props);
            if (!e) continue;

            // Wire up physics body with tilemap if present
            if (e->physics && _tilemap) {
                e->physics->setTileMap(_tilemap);
            }

            e->init();

            // Auto-tag: use entity type as tag (first spawn of that type wins)
            if (_taggedEntities.find(s.type) == _taggedEntities.end()) {
                _taggedEntities[s.type] = e;
            }
            // Explicit tag property overrides
            auto tagIt = s.props.find("tag");
            if (tagIt != s.props.end()) {
                _taggedEntities[tagIt->second] = e;
            }
        }

        // Wire camera follow
        if (!cameraFollowTag.empty()) {
            Entity* target = findByTag(cameraFollowTag);
            if (target && target->renderer) {
                _camera->follow(target->renderer, cameraSmooth);
            }
        }

        return true;
    }

    void clear() {
        // Entities are owned by EntityManager — they get swept when alive=false
        // We only clean up scene-owned resources
        _taggedEntities.clear();

        if (_camera) {
            RenderManager::getInstance().setActiveCamera(nullptr);
            delete _camera;
            _camera = nullptr;
        }
        // Tilemap entity is managed by EntityManager
        _tilemapEntity = nullptr;

        delete _tilemap;
        _tilemap = nullptr;
        delete _palette;
        _palette = nullptr;
    }
};
