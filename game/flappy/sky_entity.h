#pragma once

#include "../../engine/engine.h"
#include <vector>

class SkyEntity : public Entity {
public:
    SkyEntity() {
        renderer = new ANSIIRenderer(0, 0);
        renderer->zOrder = 0;
        renderer->screenSpace = true;

        // Fill entire screen with blue O
        std::vector<SpriteCell> cells(WIDTH * HEIGHT);
        for (auto& c : cells) {
            c.character = 'O';
            c.colorCode = 34;
        }
        renderer->loadFromData(WIDTH, HEIGHT, cells);
        // No collider
    }
};
