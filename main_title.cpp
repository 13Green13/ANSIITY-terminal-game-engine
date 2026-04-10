#include "engine/engine.h"
#include "engine/scene_loader.h"
#include "game/title/title_entity.h"

int main() {
    SceneLoader::getInstance().registerFactory("title",
        [](float x, float y, const Props& p) -> Entity* {
            return new TitleEntity(x, y, p);
        });

    SceneLoader::getInstance().load("levels/title.scene");
    Engine::getInstance().start();
    return 0;
}
