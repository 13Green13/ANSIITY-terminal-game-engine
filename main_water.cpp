#include "engine/engine.h"
#include "engine/scene_loader.h"
#include "game/water/fluid_sim_entity.h"

int main() {
    auto& scene = SceneLoader::getInstance();

    scene.registerFactory("fluid_sim", [](float x, float y, const Props& p) -> Entity* {
        return new FluidSimEntity(x, y, p);
    });

    scene.load("levels/water.scene");

    Engine::getInstance().start();

    return 0;
}
