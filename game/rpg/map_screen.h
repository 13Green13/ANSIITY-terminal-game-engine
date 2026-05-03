#pragma once

#include "screen.h"
#include "rpg_state.h"
#include "backdrop.h"
#include "../../engine/constants.h"
#include <functional>

class MapScreen : public Screen {
    RunState& _run;
    int _selected = 0;
    int _maxOptions = 0;
    std::function<void(int)> _onBattle;   // encounter index
    std::function<void()> _onMoveMgmt;

public:
    MapScreen(RunState& run, std::function<void(int)> onBattle, std::function<void()> onMoveMgmt)
        : _run(run), _onBattle(std::move(onBattle)), _onMoveMgmt(std::move(onMoveMgmt)) {}

    void build() override {
        _maxOptions = 0;

        addEntity(makeMapBackdrop());

        addEntity(new TextEntity(2, 1, "=== ENCOUNTER MAP ===", 96));

        // Hero stats
        char statBuf[128];
        snprintf(statBuf, sizeof(statBuf), "Level %d  XP: %d/%d  HP: %d  ATK: %d  DEF: %d  MAG: %d",
            _run.heroLevel, _run.heroXP, _run.heroDef.xpPerLevel,
            _run.heroStats.health, _run.heroStats.attack,
            _run.heroStats.defense, _run.heroStats.magic);
        addEntity(new TextEntity(2, 3, statBuf, 37));

        int numMonsters = (int)_run.monsters.size();
        for (int i = 0; i < numMonsters; i++) {
            float nodeX = 10.0f + i * 40.0f;
            float nodeY = 15.0f;

            bool isDefeated = _run.defeated[i];
            bool isCurrent = (i == _run.currentEncounter);
            bool isAccessible = (i <= _run.currentEncounter);

            int nameColor = isAccessible ? (isCurrent ? 93 : (isDefeated ? 32 : 37)) : 90;
            int boxColor = isCurrent ? 93 : (isDefeated ? 32 : (isAccessible ? 37 : 90));

            std::string name = _run.monsters[i].name;
            std::string status = isDefeated ? "[DEFEATED]" : (isCurrent ? "[CURRENT]" : (isAccessible ? "[REPLAY]" : "[LOCKED]"));

            std::string top = "+" + std::string(name.size() + 2, '-') + "+";
            std::string mid = "| " + name + " |";

            addEntity(new TextEntity(nodeX, nodeY, top, boxColor));
            addEntity(new TextEntity(nodeX, nodeY + 1, mid, boxColor));
            addEntity(new TextEntity(nodeX, nodeY + 2, top, boxColor));
            addEntity(new TextEntity(nodeX + 1, nodeY + 3, status, nameColor));

            if (i < numMonsters - 1) {
                float lineX = nodeX + (float)top.size() + 1;
                addEntity(new TextEntity(lineX, nodeY + 1, "---", 90));
            }

            if (isAccessible) _maxOptions++;
        }

        addEntity(new TextEntity(2, 25, "1-5: Select encounter  M: Manage moves  Q: Quit", 90));

        // Selector
        float selectorX = 10.0f + _selected * 40.0f;
        addEntity(new TextEntity(selectorX, 13.0f, "vvv", 93));
    }

    void update(float dt) override {
        auto& input = InputManager::getInstance();
        bool needRebuild = false;

        if (input.isKeyPressed(VK_LEFT) && _selected > 0) { _selected--; needRebuild = true; }
        if (input.isKeyPressed(VK_RIGHT) && _selected < _maxOptions - 1) { _selected++; needRebuild = true; }

        for (int k = 0; k < (int)_run.monsters.size(); k++) {
            if (input.isKeyPressed('1' + k) && k <= _run.currentEncounter) {
                _onBattle(k);
                return;
            }
        }

        if (input.isKeyPressed(VK_RETURN) && _selected <= _run.currentEncounter) {
            _onBattle(_selected);
            return;
        }

        if (input.isKeyPressed('M')) {
            _onMoveMgmt();
            return;
        }

        if (needRebuild) rebuild();
    }
};
