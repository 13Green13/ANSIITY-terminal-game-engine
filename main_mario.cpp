#include "engine/engine.h"
#include "engine/scene_loader.h"
#include "game/mario/player_entity.h"
#include "game/mario/goomba_entity.h"
#include "game/mario/powerup_entity.h"
#include "game/mario/finish_entity.h"

int main() {
    auto& scene = SceneLoader::getInstance();

    scene.registerFactory("player", [](float x, float y, const Props& p) -> Entity* {
        return new PlayerEntity(x, y, p);
    });
    scene.registerFactory("goomba", [](float x, float y, const Props& p) -> Entity* {
        return new GoombaEntity(x, y, p);
    });
    scene.registerFactory("powerup", [](float x, float y, const Props& p) -> Entity* {
        return new PowerupEntity(x, y, p);
    });
    scene.registerFactory("finish", [](float x, float y, const Props& p) -> Entity* {
        return new FinishEntity(x, y, p);
    });

    scene.load("levels/level1.scene");

    Engine::getInstance().start();

    return 0;
}
