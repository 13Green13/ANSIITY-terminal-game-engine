#pragma once

#include "../../engine/engine.h"
#include <string>
#include <vector>

constexpr int TIMER_HEIGHT = 3;
constexpr int TIMER_WIDTH = 3;

class CounterEntity : public Entity {
    int _score = 0;
    float _baseX;
    float _baseY;

    static inline CounterEntity* _instance = nullptr;

    struct NumberSprite {
        std::string pattern;
    };

    static NumberSprite getDigit(char digit) {
        NumberSprite n;
        switch (digit) {
            case '0': n.pattern = "NNNNONNNN"; break;
            case '1': n.pattern = "NNOONOONO"; break;
            case '2': n.pattern = "NONNONNON"; break;
            case '3': n.pattern = "NNNONNNNN"; break;
            case '4': n.pattern = "NONNNNOON"; break;
            case '5': n.pattern = "NONONONON"; break;
            case '6': n.pattern = "NOONNNNNN"; break;
            case '7': n.pattern = "NNNOONOON"; break;
            case '8': n.pattern = "NNNNNNNNN"; break;
            case '9': n.pattern = "NNNNNNOON"; break;
            default: break;
        }
        return n;
    }

public:
    CounterEntity() {
        _baseX = WIDTH / 2.0f;
        _baseY = HEIGHT / 10.0f;
        renderer = new ANSIIRenderer(_baseX, _baseY);
        renderer->zOrder = 100;
        renderer->screenSpace = true;
        renderer->width = 0;
        renderer->height = 0;
        _instance = this;
    }

    static CounterEntity* instance() { return _instance; }
    void incrementScore() { _score++; }
    int getScore() const { return _score; }

    // Counter does custom drawing — override the renderer's draw by doing it
    // directly in update and building sprite data each frame
    void update(float dt) override {
        std::string score = std::to_string(_score);

        // Build the cells for all digits side by side
        std::vector<SpriteCell> cells;
        int totalWidth = 0;

        for (int k = 0; k < (int)score.length(); k++) {
            if (k > 0) {
                // Add spacing column
                for (int i = 0; i < TIMER_HEIGHT; i++) {
                    SpriteCell spacer;
                    spacer.character = ' ';
                    spacer.colorCode = 0;
                    cells.push_back(spacer);
                }
                totalWidth++;
            }
            NumberSprite ns = getDigit(score[k]);
            // We need to interleave columns, but renderer stores row-major
            // So we just store per-digit and set renderer data
        }

        // Simpler approach: manually draw in the screen buffers
        // We'll override renderer position each frame
        renderer->x = _baseX;
        renderer->y = _baseY;

        // Build a flat sprite: digits side by side with 1-cell gap
        int digitCount = (int)score.length();
        int fullWidth = digitCount * TIMER_WIDTH + (digitCount - 1);
        int fullHeight = TIMER_HEIGHT;

        std::vector<SpriteCell> allCells(fullWidth * fullHeight);
        // Init to blank
        for (auto& c : allCells) {
            c.character = ' ';
            c.colorCode = 0;
        }

        int xOff = 0;
        for (int k = 0; k < digitCount; k++) {
            NumberSprite ns = getDigit(score[k]);
            int idx = 0;
            for (int row = 0; row < TIMER_HEIGHT; row++) {
                for (int col = 0; col < TIMER_WIDTH; col++) {
                    char ch = ns.pattern[idx++];
                    int cellIdx = row * fullWidth + (xOff + col);
                    if (ch == 'N') {
                        allCells[cellIdx].character = 'N';
                        allCells[cellIdx].colorCode = 37;
                    } else {
                        allCells[cellIdx].character = ' ';
                        allCells[cellIdx].colorCode = 0;
                    }
                }
            }
            xOff += TIMER_WIDTH + 1;
        }

        renderer->loadFromData(fullWidth, fullHeight, allCells);
    }
};
