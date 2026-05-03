#include "engine/engine.h"
#include "engine/scene_loader.h"

class SpriteTestEntity : public Entity {
public:
    SpriteTestEntity(float x, float y, const std::string& file) {
        renderer = new ANSIIRenderer(x, y);
        renderer->zOrder = 10;
        renderer->screenSpace = true;
        renderer->loadFromFile(file);
    }
};

int main() {
    const char* sprites[] = {
        "sprites/knight.ansii",
        "sprites/dwarf.ansii",
        "sprites/mage.ansii",
        "sprites/priest.ansii",
        "sprites/goblin.ansii",
        "sprites/goblinMage.ansii",
        "sprites/spider.ansii",
        "sprites/witch.ansii",
        "sprites/dragon.ansii",
    };

    float x = 2;
    float y = 2;
    float rowHeight = 0;

    for (auto* file : sprites) {
        auto* e = new SpriteTestEntity(x, y, file);
        e->init();

        float w = (float)e->renderer->width;
        float h = (float)e->renderer->height;

        if (h > rowHeight) rowHeight = h;

        x += w + 3;

        // Wrap to next row if we'd go off screen
        if (x + 35 > WIDTH) {
            x = 2;
            y += rowHeight + 2;
            rowHeight = 0;
        }
    }

    InputManager::getInstance().watchKey('Q');
    Engine::getInstance().start();
    return 0;
}
