#pragma once

#include "screen.h"
#include "rpg_state.h"
#include "char_select_screen.h"  // for buildMoveInfo
#include "backdrop.h"
#include "../../engine/constants.h"
#include <functional>

class MoveMgmtScreen : public Screen {
    RunState& _run;
    std::function<void()> _onBack;
    int _cursor = 0;
    int _slot = -1; // -1 = none selected, 0-3 = equip slot

public:
    MoveMgmtScreen(RunState& run, std::function<void()> onBack)
        : _run(run), _onBack(std::move(onBack)) {}

    void build() override {
        addEntity(makeMoveMgmtBackdrop());

        addEntity(new TextEntity(2, 1, "=== MOVE MANAGEMENT ===", 96));
        addEntity(new TextEntity(2, 3, "Equipped (4 slots):", 97));

        for (int i = 0; i < 4; i++) {
            char label[64];
            snprintf(label, sizeof(label), "[%d] %s", i + 1, _run.equippedMoves[i].name.c_str());
            int color = (_slot == i) ? 93 : 37;
            addEntity(new TextEntity(4, (float)(5 + i), label, color));
        }

        addEntity(new TextEntity(2, 11, "Learned moves:", 97));
        for (int i = 0; i < (int)_run.learnedMoves.size(); i++) {
            char label[64];
            snprintf(label, sizeof(label), "  %s", _run.learnedMoves[i].name.c_str());
            int color = (i == _cursor) ? 93 : 37;
            if (i + 13 < HEIGHT - 3) {
                addEntity(new TextEntity(4, (float)(13 + i), label, color));
            }
        }

        addEntity(new TextEntity(2, (float)(HEIGHT - 3),
            "UP/DOWN: navigate  1-4: select equip slot  ENTER: swap  ESC: back to map", 90));

        // ── Move info panel (right side) ──
        float infoX = 120;

        // Show info for equipped slot if one is selected
        if (_slot >= 0 && _slot < 4) {
            addEntity(new TextEntity(infoX, 3, "--- EQUIPPED SLOT ---", 96));
            addEntity(new TextEntity(infoX, 5, _run.equippedMoves[_slot].name, 93));
            buildMoveInfo(*this, infoX + 2, 6, _run.equippedMoves[_slot]);
        }

        // Show info for highlighted learned move
        if (_cursor >= 0 && _cursor < (int)_run.learnedMoves.size()) {
            float poolInfoY = (_slot >= 0) ? 14.0f : 3.0f;
            addEntity(new TextEntity(infoX, poolInfoY, "--- SELECTED MOVE ---", 96));
            addEntity(new TextEntity(infoX, poolInfoY + 2, _run.learnedMoves[_cursor].name, 93));
            buildMoveInfo(*this, infoX + 2, poolInfoY + 3, _run.learnedMoves[_cursor]);
        }
    }

    void update(float dt) override {
        auto& input = InputManager::getInstance();
        bool needRebuild = false;

        if (input.isKeyPressed(VK_UP) && _cursor > 0) { _cursor--; needRebuild = true; }
        if (input.isKeyPressed(VK_DOWN) && _cursor < (int)_run.learnedMoves.size() - 1) { _cursor++; needRebuild = true; }

        for (int k = 0; k < 4; k++) {
            if (input.isKeyPressed('1' + k)) { _slot = k; needRebuild = true; }
        }

        if (input.isKeyPressed(VK_RETURN) && _slot >= 0 && _slot < 4
            && _cursor >= 0 && _cursor < (int)_run.learnedMoves.size()) {
            _run.equippedMoves[_slot] = _run.learnedMoves[_cursor];
            _slot = -1;
            needRebuild = true;
        }

        if (input.isKeyPressed(VK_ESCAPE)) {
            _onBack();
            return;
        }

        if (needRebuild) rebuild();
    }
};
