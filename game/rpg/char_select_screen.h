#pragma once

#include "screen.h"
#include "rpg_state.h"
#include "http_client.h"
#include "../../engine/constants.h"
#include <functional>

// ─── Reusable move info builder ─────────────────────────────────────
// addFn is a callable that takes Entity* and registers it (allows different
// tracking lists, e.g. Screen::addEntity vs BattleScreen::addUI)

template<typename AddFn>
inline void buildMoveInfoWith(AddFn addFn, float x, float y, const Move& m) {
    // Line 1: Type
    std::string typeStr = "Type: " + m.type;
    int typeColor = (m.type == "physical") ? 33 : (m.type == "magic") ? 35 : 37;
    addFn(new TextEntity(x, y, typeStr, typeColor));

    // Line 2: Effect
    addFn(new TextEntity(x, y + 1, "Effect: " + m.effect, 37));

    // Line 3+: Values depending on effect
    float line = y + 2;
    if (m.baseValue > 0) {
        std::string label = (m.effect == "heal") ? "Heal Base: " : "Base Dmg: ";
        addFn(new TextEntity(x, line, label + std::to_string(m.baseValue), 97));
        line++;
    }
    if (!m.buffStat.empty()) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Buff: %s +%d (%dt)", m.buffStat.c_str(), m.buffAmount, m.buffTurns);
        addFn(new TextEntity(x, line, buf, 36));
        line++;
    }
    if (!m.debuffStat.empty()) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Debuff: %s -%d (%dt)", m.debuffStat.c_str(), m.debuffAmount, m.debuffTurns);
        addFn(new TextEntity(x, line, buf, 35));
        line++;
    }
    if (m.selfDamage > 0) {
        addFn(new TextEntity(x, line, "Self dmg: " + std::to_string(m.selfDamage), 91));
    }
}

inline void buildMoveInfo(Screen& scr, float x, float y, const Move& m) {
    buildMoveInfoWith([&scr](Entity* e) { scr.addEntity(e); }, x, y, m);
}

// ─── Character Select Screen ────────────────────────────────────────

class CharSelectScreen : public Screen {
    RunState& _run;
    HttpClient& _http;
    int _selected = 0;
    std::function<void()> _onConfirm;
    std::function<void()> _onBack;

public:
    CharSelectScreen(RunState& run, HttpClient& http,
                     std::function<void()> onConfirm,
                     std::function<void()> onBack)
        : _run(run), _http(http),
          _onConfirm(std::move(onConfirm)), _onBack(std::move(onBack)) {}

    void build() override {
        addEntity(makeCharSelectBackdrop());

        addEntity(new TextEntity(2, 1, "=== CHOOSE YOUR HERO ===", 96));
        addEntity(new TextEntity(2, 3, "LEFT/RIGHT: browse   ENTER: confirm   ESC: back", 90));

        int numHeroes = (int)_run.heroChoices.size();
        if (numHeroes == 0) return;

        auto& hero = _run.heroChoices[_selected];

        // Hero sprite (centered)
        float spriteX = 30;
        float spriteY = 8;
        addEntity(new SpriteEntity(spriteX, spriteY, hero.sprite, 10));

        // Name
        addEntity(new TextEntity(spriteX, spriteY - 2, hero.name, 93));

        // Selection arrows
        if (_selected > 0)
            addEntity(new TextEntity(spriteX - 5, spriteY + 6, "<", 93));
        if (_selected < numHeroes - 1)
            addEntity(new TextEntity(spriteX + 35, spriteY + 6, ">", 93));

        // Character indicator dots
        std::string dots;
        for (int i = 0; i < numHeroes; i++) {
            dots += (i == _selected) ? "@ " : "o ";
        }
        addEntity(new TextEntity(spriteX + 10, spriteY + 18, dots, 97));

        // Stats panel (right side)
        float statX = 90;
        float statY = 8;
        addEntity(new TextEntity(statX, statY, "--- BASE STATS ---", 96));

        char buf[64];
        snprintf(buf, sizeof(buf), "HP:  %d", hero.baseStats.health);
        addEntity(new TextEntity(statX, statY + 2, buf, 32));
        snprintf(buf, sizeof(buf), "ATK: %d", hero.baseStats.attack);
        addEntity(new TextEntity(statX, statY + 3, buf, 33));
        snprintf(buf, sizeof(buf), "DEF: %d", hero.baseStats.defense);
        addEntity(new TextEntity(statX, statY + 4, buf, 36));
        snprintf(buf, sizeof(buf), "MAG: %d", hero.baseStats.magic);
        addEntity(new TextEntity(statX, statY + 5, buf, 35));

        addEntity(new TextEntity(statX, statY + 7, "--- GROWTH /LVL ---", 96));
        snprintf(buf, sizeof(buf), "HP:  +%d", hero.statGrowth.health);
        addEntity(new TextEntity(statX, statY + 9, buf, 32));
        snprintf(buf, sizeof(buf), "ATK: +%d", hero.statGrowth.attack);
        addEntity(new TextEntity(statX, statY + 10, buf, 33));
        snprintf(buf, sizeof(buf), "DEF: +%d", hero.statGrowth.defense);
        addEntity(new TextEntity(statX, statY + 11, buf, 36));
        snprintf(buf, sizeof(buf), "MAG: +%d", hero.statGrowth.magic);
        addEntity(new TextEntity(statX, statY + 12, buf, 35));

        // Starting moves (below stats)
        float moveListY = statY + 15;
        addEntity(new TextEntity(statX, moveListY, "--- STARTING MOVES ---", 96));
        for (int i = 0; i < (int)hero.moves.size(); i++) {
            float my = moveListY + 2 + i * 7;
            auto& m = hero.moves[i];
            addEntity(new TextEntity(statX, my, m.name, 97));
            buildMoveInfo(*this, statX + 2, my + 1, m);
        }
    }

    void update(float dt) override {
        auto& input = InputManager::getInstance();
        int numHeroes = (int)_run.heroChoices.size();
        bool needRebuild = false;

        if (input.isKeyPressed(VK_LEFT) && _selected > 0) { _selected--; needRebuild = true; }
        if (input.isKeyPressed(VK_RIGHT) && _selected < numHeroes - 1) { _selected++; needRebuild = true; }

        if (input.isKeyPressed(VK_RETURN)) {
            // Set chosen hero and re-init run
            _run.heroDef = _run.heroChoices[_selected];
            _run.init();
            _onConfirm();
            return;
        }

        if (input.isKeyPressed(VK_ESCAPE)) {
            _onBack();
            return;
        }

        if (needRebuild) rebuild();
    }
};
