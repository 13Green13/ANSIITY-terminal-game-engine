#include "engine/engine.h"
#include "engine/scene_loader.h"
#include "game/asteroids/ship_entity.h"
#include "game/asteroids/spawner_entity.h"
#include "game/asteroids/score_entity.h"
#include "game/asteroids/game_state.h"

int main() {
    auto& scene = SceneLoader::getInstance();

    scene.registerFactory("ship", [](float x, float y, const Props& p) -> Entity* {
        return new ShipEntity(x, y, p);
    });
    scene.registerFactory("spawner", [](float x, float y, const Props& p) -> Entity* {
        return new SpawnerEntity(x, y, p);
    });
    scene.registerFactory("score_hud", [](float x, float y, const Props& p) -> Entity* {
        return new ScoreEntity(x, y, p);
    });

    GameState::reset();
    scene.load("levels/asteroids.scene");

    Engine::getInstance().start();

    return 0;
}
