#include "engine/engine.h"
#include "engine/scene_loader.h"
#include "game/cavern/miner_entity.h"
#include "game/cavern/bat_entity.h"
#include "game/cavern/gem_entity.h"
#include "game/cavern/exit_entity.h"
#include "game/cavern/hud_entity.h"

int main() {
    Profiler::getInstance().enable("perf_log.txt", 60);

    auto& scene = SceneLoader::getInstance();

    scene.registerFactory("miner", [](float x, float y, const Props& p) -> Entity* {
        return new MinerEntity(x, y, p);
    });
    scene.registerFactory("bat", [](float x, float y, const Props& p) -> Entity* {
        return new BatEntity(x, y, p);
    });
    scene.registerFactory("gem_cyan", [](float x, float y, const Props& p) -> Entity* {
        return new GemCyanEntity(x, y, p);
    });
    scene.registerFactory("gem_green", [](float x, float y, const Props& p) -> Entity* {
        return new GemGreenEntity(x, y, p);
    });
    scene.registerFactory("gem_red", [](float x, float y, const Props& p) -> Entity* {
        return new GemRedEntity(x, y, p);
    });
    scene.registerFactory("exit_door", [](float x, float y, const Props& p) -> Entity* {
        return new ExitDoorEntity(x, y, p);
    });
    scene.registerFactory("hud", [](float x, float y, const Props& p) -> Entity* {
        return new HudEntity(x, y, p);
    });

    CavernState::reset();
    scene.load("levels/cavern.scene");

    Engine::getInstance().start();

    return 0;
}