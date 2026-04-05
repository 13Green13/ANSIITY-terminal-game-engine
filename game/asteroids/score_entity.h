#pragma once

#include "../../engine/engine.h"
#include "../../engine/constants.h"
#include "game_state.h"
#include <string>

class ScoreEntity : public Entity {
    int _lastScore = -1;
    int _lastWave = -1;

    void rebuildSprite() {
        std::string text = "SCORE: " + std::to_string(GameState::score)
                         + "  WAVE: " + std::to_string(GameState::wave);

        std::vector<SpriteCell> cells;
        for (char c : text) {
            cells.push_back({c, 97});  // bright white
        }
        renderer->loadFromData((int)text.size(), 1, cells);
    }

public:
    ScoreEntity(float x, float y, const Props&) {
        tag = "score_hud";

        renderer = new ANSIIRenderer(x, y);
        renderer->zOrder = 100;
        renderer->screenSpace = true;

        // No collider — HUD only
    }

    void update(float dt) override {
        if (GameState::score != _lastScore || GameState::wave != _lastWave) {
            _lastScore = GameState::score;
            _lastWave = GameState::wave;
            rebuildSprite();
        }
    }
};
