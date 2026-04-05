# ANSIITY

A lightweight 2D ASCII / ANSI game engine written in C++20.

Build games like Flappy Bird, Mario, and physics simulations — entirely inside the terminal.

![demo](docs/demo.gif)

## Features

- ANSI sprite rendering with color and transparency
- Entity-component system with optional physics, colliders, and renderers
- Grid-based collision detection and DDA raycasting
- Tilemap and tile-palette system with runtime modification
- Camera with target following, smoothing, and dead zones
- Double-buffered single-syscall rendering (no flicker)
- Data-driven scene loader with entity factories
- Built-in per-frame profiler with file logging
- Header-only architecture — no build system required

## Requirements

- Windows 10+ with Windows Terminal
- MinGW GCC 15+ (or any C++20 compiler)
- Python 3 (only for level generation scripts)

## Building

Each game has its own `main_<name>.cpp` and needs a specific screen resolution.
Override `engine/constants.h` defaults at compile time with `-DSCREEN_WIDTH=N -DSCREEN_HEIGHT=N`:

### Example-Games

| Game | Source | Resolution | Controls |
|------|--------|------------|----------|
| Flappy Birb | `main_flappy.cpp` | 120x60 | Space = jump |
| Mario | `main_mario.cpp` | 240x60 | Arrows = move, Space = jump |
| Asteroids | `main_asteroids.cpp` | 240x60 | Arrows = rotate/thrust, Space = shoot |
| Water Sim | `main_water.cpp` | 240x60 | Arrows = move cursor, Space = pour |
| Crystal Caverns | `main_cavern.cpp` | 400x100 | Arrows = move/jetpack, X = bomb |

Build all:
```
g++ -O2 -o build/flappy.exe main_flappy.cpp -std=c++20 -I. -DSCREEN_WIDTH=120 -DSCREEN_HEIGHT=60
g++ -O2 -o build/mario.exe main_mario.cpp -std=c++20 -I. -DSCREEN_WIDTH=240 -DSCREEN_HEIGHT=60
g++ -O2 -o build/asteroids.exe main_asteroids.cpp -std=c++20 -I. -DSCREEN_WIDTH=240 -DSCREEN_HEIGHT=60
g++ -O2 -o build/water.exe main_water.cpp -std=c++20 -I. -DSCREEN_WIDTH=240 -DSCREEN_HEIGHT=60
g++ -O2 -o build/cavern.exe main_cavern.cpp -std=c++20 -I. -DSCREEN_WIDTH=400 -DSCREEN_HEIGHT=100
```

## Project Structure

```
engine/          Header-only engine (entity, physics, rendering, collision, etc.)
game/
  flappy/        Flappy Bird entities
  mario/         Mario platformer entities
  asteroids/     Asteroids entities
  water/         SPH fluid simulation
  cavern/        Crystal Caverns entities
levels/          Scene files, tilemaps, and tile palettes
sprites/         .ansii sprite files
gen_level.py     Mario level generator
gen_cavern.py    Crystal Caverns procedural cave generator
```

## Engine Overview

- **Entity-Component** pattern with optional renderer, collider, and physics body
- **Singleton managers**: EntityManager, CollisionManager, RenderManager, InputManager
- **Camera** with dead zone, lerp smoothing, and optional bounds clamping
- **TileMap** with palette-driven rendering and O(1) solid checks
- **PhysicsBody** with force/acceleration model and per-axis tile collision
- **SceneLoader** for declarative level files with entity factories
- **Profiler** with per-frame timing breakdown

See [design.txt](design.txt) for full architecture details.

---

## API Reference

### Constants — `engine/constants.h`

| Constant | Default | Override |
|----------|---------|----------|
| `WIDTH` (screen columns) | `240` | `-DSCREEN_WIDTH=N` |
| `HEIGHT` (screen rows) | `60` | `-DSCREEN_HEIGHT=N` |
| `TARGET_FPS` | `60` | — |
| `TRANSPARENT_COLOR` | `0` | — |

### Entity — `engine/entity.h`

Base class for all game objects. Subclass and override to create game entities.

**Fields:**
| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `alive` | `bool` | `true` | Set to `false` to destroy next frame |
| `tag` | `std::string` | `""` | Tag for identification in collisions/lookups |
| `renderer` | `ANSIIRenderer*` | `nullptr` | Optional sprite renderer |
| `collider` | `ANSIICollider*` | `nullptr` | Optional collider |
| `physics` | `PhysicsBody*` | `nullptr` | Optional physics body |

**Virtual methods:**
| Signature | Description |
|-----------|-------------|
| `void update(float dt)` | Called every frame |
| `void onCollision(Entity* other, const CollisionInfo& info)` | Called on collider overlap |
| `void onTileCollision(const PhysicsBody::TileCollision& tc)` | Called when physics hits a solid tile |

**Lifecycle:**
| Signature | Description |
|-----------|-------------|
| `void init()` | Registers renderer/collider/physics with managers — call after construction |
| `void destroy()` | Marks `alive = false` for sweep |

**CollisionInfo struct:**
| Field | Type | Description |
|-------|------|-------------|
| `normalX`, `normalY` | `float` | Collision normal direction |
| `overlapX`, `overlapY` | `float` | Penetration depth per axis |

### ANSIIRenderer — `engine/ansii_renderer.h`

Sprite renderer using ANSI color codes. Each cell has a character + color.

**Fields:**
| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `x`, `y` | `float` | `0` | World position |
| `width`, `height` | `int` | `0` | Sprite dimensions |
| `zOrder` | `int` | `0` | Draw order (higher = further back) |
| `screenSpace` | `bool` | `false` | If `true`, ignores camera offset (for HUD) |

**Methods:**
| Signature | Description |
|-----------|-------------|
| `void loadFromFile(const std::string& path)` | Load an `.ansii` sprite file |
| `void loadFromData(const std::vector<std::string>& lines)` | Load from string vector (inline sprites) |
| `void draw(CHAR_INFO* buffer, int bufW, int bufH, float camX, float camY) const` | Renders to buffer (called by RenderManager) |

Color codes: standard ANSI 30–37 and bright 90–97. Color `0` = transparent (skipped).

### ANSIICollider — `engine/ansii_collider.h`

Axis-aligned bounding box or circle collider with layer filtering.

**Fields:**
| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `id` | `int` | auto | Unique ID |
| `layer` | `int` | `0` | Collision layer (0–31) |
| `layerMask` | `int` | `-1` | Bitmask of layers to collide with (`-1` = all) |
| `owner` | `Entity*` | — | Entity that owns this collider |

**Constructors:**
| Signature | Description |
|-----------|-------------|
| `ANSIICollider(ANSIIRenderer* renderer, Entity* owner)` | AABB from renderer bounds |
| `ANSIICollider(float radius, ANSIIRenderer* renderer, Entity* owner)` | Circle with radius |

**Methods:**
| Signature | Description |
|-----------|-------------|
| `void setCircle(float radius)` | Switch to circle shape |
| `bool shouldCollideWith(const ANSIICollider& other) const` | Layer mask check |

### PhysicsBody — `engine/physics_body.h`

Force-and-acceleration physics with tile collision response.

**Fields:**
| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `velX`, `velY` | `float` | `0` | Current velocity |
| `accelX`, `accelY` | `float` | `0` | Acceleration (persistent, e.g. gravity) |
| `dragX`, `dragY` | `float` | `0` | Velocity damping per frame |
| `maxVelX`, `maxVelY` | `float` | `999` | Velocity caps |
| `grounded` | `bool` | `false` | True if touching solid tile below |
| `angle`, `angularVel`, `angularAccel`, `angularDrag` | `float` | `0` | Rotational physics |

**Methods:**
| Signature | Description |
|-----------|-------------|
| `void setTileMap(TileMap* map)` | Enable tile collision |
| `void addForce(float fx, float fy)` | Instantaneous impulse |
| `void addTorque(float t)` | Angular impulse |
| `void setAccel(float ax, float ay)` | Override persistent acceleration |
| `void update(float dt)` | Integrate motion and resolve tile collisions |

**TileCollision struct:**
| Field | Type | Description |
|-------|------|-------------|
| `side` | `int` | 0=bottom, 1=top, 2=left, 3=right |
| `tileX`, `tileY` | `int` | World position of the tile |
| `tileId` | `uint8_t` | Tile palette ID |

### InputManager — `engine/input_manager.h` (Singleton)

Polls keyboard state via `GetAsyncKeyState`. Access: `InputManager::getInstance()`

**Methods:**
| Signature | Description |
|-----------|-------------|
| `void watchKey(int vk)` | Register a key to track (use Windows VK codes) |
| `void poll()` | Update all watched keys (called by engine each frame) |
| `bool isKeyHeld(int vk) const` | Key is currently down |
| `bool isKeyPressed(int vk) const` | Key just pressed this frame |
| `bool isKeyReleased(int vk) const` | Key just released this frame |

**Note:** `'Q'` is reserved by the engine for quit. Use VK codes like `VK_LEFT`, `VK_SPACE`, `'A'`, etc.

### Camera — `engine/camera.h`

Scrolling camera with target following, smoothing, and dead zones. **Not** a singleton — created per scene.

**Fields:**
| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `x`, `y` | `float` | `0` | World position (top-left corner of view) |
| `smoothing` | `float` | `0` | Lerp factor (0 = snap, higher = slower drift) |
| `deadZoneW`, `deadZoneH` | `float` | `0` | Dead zone dimensions |

**Methods:**
| Signature | Description |
|-----------|-------------|
| `void follow(ANSIIRenderer* target, float smooth = 0)` | Track a renderer |
| `void unfollow()` | Stop following |
| `void update(float dt)` | Called by engine each frame |
| `bool isVisible(float wx, float wy, int w, int h) const` | Frustum culling check |

### EntityManager — `engine/entity_manager.h` (Singleton)

Owns and updates all entities. Access: `EntityManager::getInstance()`

| Signature | Description |
|-----------|-------------|
| `void subscribe(Entity* entity)` | Register entity (called by `Entity::init()`) |
| `void update(float dt)` | Update all alive entities, sweep dead ones |
| `const std::vector<Entity*>& getEntities() const` | Access all entities |

### CollisionManager — `engine/collision_manager.h` (Singleton)

Grid-based collision with layer filtering and raycasting. Access: `CollisionManager::getInstance()`

**Methods:**
| Signature | Description |
|-----------|-------------|
| `void setWorldSize(int w, int h)` | Resize collision grid |
| `void subscribe(ANSIICollider* c)` | Register collider |
| `void unsubscribe(ANSIICollider* c)` | Remove collider |
| `void resolve()` | Run collision detection |
| `RayHit raycast(float ox, float oy, float dx, float dy, float maxDist = 500, int layerMask = -1)` | DDA raycast with direction vector |
| `RayHit raycastAngle(float ox, float oy, float angleDeg, float maxDist = 500, int layerMask = -1)` | Raycast by angle (0=down, 90=right, 180=up, 270=left) |

**RayHit struct:** `hit` (bool), `x`/`y` (float), `distance` (float), `entity` (Entity*). Converts to bool via `operator bool()`.

### RenderManager — `engine/render_manager.h` (Singleton)

Composites all renderers to console output. Access: `RenderManager::getInstance()`

| Signature | Description |
|-----------|-------------|
| `void subscribe(ANSIIRenderer* r)` | Register renderer (sorted by zOrder) |
| `void unsubscribe(ANSIIRenderer* r)` | Remove renderer |
| `void setActiveCamera(Camera* cam)` | Set camera for world-space offset |
| `Camera* getActiveCamera() const` | Get active camera |
| `void draw()` | Composite and flush to console |

### TileMap & TilePalette — `engine/tilemap.h`

**TileDef struct:** `character` (char), `colorCode` (int), `solid` (bool).

**TilePalette:**
| Signature | Description |
|-----------|-------------|
| `bool loadFromFile(const std::string& path)` | Load `.tilepalette` file |
| `const TileDef& get(uint8_t id) const` | Get tile definition (0–255) |

**TileMap:**
| Signature | Description |
|-----------|-------------|
| `void setPalette(TilePalette* p)` | Assign a palette |
| `bool loadFromFile(const std::string& path)` | Load `.tilemap` file |
| `uint8_t getTile(int wx, int wy) const` | Get tile ID at world position |
| `bool isSolid(int wx, int wy) const` | Check if tile blocks physics |
| `void setTile(int wx, int wy, uint8_t id)` | Modify a tile at runtime |
| `int getMapWidth() const` / `int getMapHeight() const` | Map dimensions in tiles |

Tilemap encoding: `0-9` → IDs 0–9, `a-z` → 10–35, `A-Z` → 36–61.

### TileMapRenderer — `engine/tilemap_renderer.h`

Extends `ANSIIRenderer`. Draws only the camera-visible rectangle of a `TileMap`.

```cpp
TileMapRenderer(TileMap* tileMap)  // sets dimensions to full map size
```

### SceneLoader — `engine/scene_loader.h` (Singleton)

Data-driven scene loading from `.scene` files. Access: `SceneLoader::getInstance()`

**Methods:**
| Signature | Description |
|-----------|-------------|
| `void registerFactory(const std::string& type, EntityFactory factory)` | Register entity type for scene spawning |
| `bool load(const std::string& filepath)` | Load a `.scene` file |
| `void clear()` | Clean up scene resources |
| `TileMap* getTileMap() const` | Get current tilemap |
| `Camera* getCamera() const` | Get current camera |
| `Entity* findByTag(const std::string& tag)` | Lookup tagged entity |

**Scene file keywords:**
```
tilemap <path>                          # load tilemap
palette <path>                          # load tile palette
spawn <type> <x> <y> [key=value...]     # spawn entity with optional props
camera_follow <tag>                     # follow entity by tag
camera_smooth <float>                   # smoothing factor
camera_deadzone <w> <h>                 # dead zone size
camera_start <x> <y>                    # initial position
```

**Factory type alias:**
```cpp
using Props = std::unordered_map<std::string, std::string>;
using EntityFactory = std::function<Entity*(float, float, const Props&)>;
```

### Profiler — `engine/profiler.h` (Singleton)

Per-frame performance profiler with file logging. Access: `Profiler::getInstance()`

| Signature | Description |
|-----------|-------------|
| `void enable(const std::string& logPath = "perf_log.txt", int logEveryN = 60)` | Enable profiling |
| `void beginFrame()` / `void endFrame(int entityCount)` | Frame timing |
| `void beginSection()` / `float endSection()` | Section timing (returns µs) |
| `void recordInput/Update/Collision/Render(float us)` | Phase timings |
| `void writeSummary()` | Flush final stats to log |

### Engine — `engine/engine.h` (Singleton)

Main game loop. Access: `Engine::getInstance()`

| Signature | Description |
|-----------|-------------|
| `void start()` | Enter game loop (blocks until `stop()` or Q) |
| `void stop()` | Exit the game loop |

**Frame loop order:** Input → Update → Physics → Camera → Collision → Render → Sleep(16ms)

Console auto-scales font to fit the configured `WIDTH × HEIGHT`.

### Singleton Summary

| Class | Access |
|-------|--------|
| `Engine` | `Engine::getInstance()` |
| `InputManager` | `InputManager::getInstance()` |
| `EntityManager` | `EntityManager::getInstance()` |
| `CollisionManager` | `CollisionManager::getInstance()` |
| `RenderManager` | `RenderManager::getInstance()` |
| `SceneLoader` | `SceneLoader::getInstance()` |
| `Profiler` | `Profiler::getInstance()` |

---

## Quick-Start Example

```cpp
#include "engine/engine.h"
#include "engine/scene_loader.h"
#include "engine/input_manager.h"
#include "game/mario/player_entity.h"

int main() {
    // Register entity factories for scene files
    SceneLoader::getInstance().registerFactory("player", [](float x, float y, const auto& props) {
        return new PlayerEntity(x, y);
    });

    // Load a scene (tilemap + palette + entity spawns + camera)
    SceneLoader::getInstance().load("levels/mario/level1.scene");

    // Run the game loop (blocks until Q or Engine::stop())
    Engine::getInstance().start();

    // Cleanup
    SceneLoader::getInstance().clear();
    return 0;
}
```

### Creating an Entity

```cpp
#include "engine/entity.h"

struct MyEntity : Entity {
    MyEntity(float x, float y) {
        renderer = new ANSIIRenderer();
        renderer->loadFromFile("sprites/my_sprite.ansii");
        renderer->x = x;
        renderer->y = y;

        collider = new ANSIICollider(renderer, this);
        collider->layer = 1;

        physics = new PhysicsBody(renderer);
        physics->accelY = 30.0f;  // gravity
        physics->dragX = 0.85f;

        InputManager::getInstance().watchKey(VK_LEFT);
        InputManager::getInstance().watchKey(VK_RIGHT);
        InputManager::getInstance().watchKey(VK_SPACE);
    }

    void update(float dt) override {
        auto& input = InputManager::getInstance();
        if (input.isKeyHeld(VK_LEFT))  physics->addForce(-50, 0);
        if (input.isKeyHeld(VK_RIGHT)) physics->addForce(50, 0);
        if (input.isKeyPressed(VK_SPACE) && physics->grounded)
            physics->velY = -20.0f;
    }

    void onCollision(Entity* other, const CollisionInfo& info) override {
        if (other->tag == "coin") {
            other->destroy();
        }
    }
};
```

---

## Keywords

C++ ASCII game engine, ANSI renderer, terminal game engine, console rendering,
text-based game development, header-only, Windows console, tilemap, raycasting,
entity-component system.
