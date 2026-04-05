#pragma once

#include "../../engine/engine.h"
#include "../../engine/constants.h"
#include "game_state.h"
#include <string>

class HudEntity : public Entity {
    int _lastGems = -1;
    int _lastTotal = -1;

    void rebuildSprite() {
        std::string gemsStr = std::to_string(CavernState::gems);
        std::string totalStr = std::to_string(CavernState::totalGems);

        // "GEMS: 12/53  [Find all gems to unlock the exit]"
        std::string text;
        bool allFound = (CavernState::gems >= CavernState::totalGems);
        if (allFound) {
            text = " GEMS: " + gemsStr + "/" + totalStr + "  EXIT UNLOCKED! ";
        } else {
            text = " GEMS: " + gemsStr + "/" + totalStr + " ";
        }

        std::vector<SpriteCell> cells;
        for (int i = 0; i < (int)text.size(); i++) {
            char c = text[i];
            int color;
            if (i >= 7 && i < 7 + (int)gemsStr.size()) {
                color = allFound ? 92 : 96;  // bright green if done, bright cyan
            } else if (allFound && i >= (int)text.size() - 17) {
                color = 92;  // bright green for "EXIT UNLOCKED!"
            } else {
                color = 97;  // bright white
            }
            cells.push_back({c, color});
        }
        renderer->loadFromData((int)text.size(), 1, cells);
    }

public:
    HudEntity(float x, float y, const Props&) {
        tag = "hud";

        renderer = new ANSIIRenderer(x, y);
        renderer->zOrder = 100;
        renderer->screenSpace = true;
    }

    void update(float dt) override {
        if (CavernState::gems != _lastGems || CavernState::totalGems != _lastTotal) {
            _lastGems = CavernState::gems;
            _lastTotal = CavernState::totalGems;
            rebuildSprite();
        }
    }
};
