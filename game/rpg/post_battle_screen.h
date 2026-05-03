#pragma once

#include "screen.h"
#include "rpg_state.h"
#include "backdrop.h"
#include "../../engine/constants.h"
#include <functional>

class PostBattleScreen : public Screen {
    RunState& _run;
    bool _heroWon;
    int _monsterIdx;
    std::string _learnedMoveName;
    std::function<void()> _onContinue;
    std::function<void()> _onWin;

public:
    PostBattleScreen(RunState& run, bool heroWon, int monsterIdx,
                     const std::string& learnedMove,
                     std::function<void()> onContinue,
                     std::function<void()> onWin)
        : _run(run), _heroWon(heroWon), _monsterIdx(monsterIdx),
          _learnedMoveName(learnedMove),
          _onContinue(std::move(onContinue)), _onWin(std::move(onWin)) {}

    void build() override {
        addEntity(makePostBattleBackdrop(_heroWon));

        float cx = (float)(WIDTH / 2);
        float cy = (float)(HEIGHT / 3);

        if (_heroWon) {
            addEntity(new TextEntity(cx - 8, cy, "=== VICTORY! ===", 92));

            auto& monster = _run.monsters[_monsterIdx];
            addEntity(new TextEntity(cx - 15, cy + 3, "Defeated " + monster.name + "!", 97));

            int xp = _monsterIdx < (int)_run.xpRewards.size() ? _run.xpRewards[_monsterIdx] : 0;
            char xpBuf[64];
            snprintf(xpBuf, sizeof(xpBuf), "Gained %d XP! (Level %d)", xp, _run.heroLevel);
            addEntity(new TextEntity(cx - 15, cy + 5, xpBuf, 33));

            if (!_learnedMoveName.empty()) {
                addEntity(new TextEntity(cx - 15, cy + 7, "Learned new move: " + _learnedMoveName + "!", 96));
            } else {
                addEntity(new TextEntity(cx - 15, cy + 7, "No new moves to learn from this monster.", 90));
            }

            if (_run.allDefeated) {
                addEntity(new TextEntity(cx - 15, cy + 10, "Press ENTER to see your victory!", 93));
            } else {
                addEntity(new TextEntity(cx - 15, cy + 10, "Press ENTER to continue", 90));
            }
        } else {
            addEntity(new TextEntity(cx - 8, cy, "=== DEFEATED ===", 91));
            addEntity(new TextEntity(cx - 15, cy + 3, "You were slain...", 37));
            addEntity(new TextEntity(cx - 15, cy + 5, "Press ENTER to return to map", 90));
        }
    }

    void update(float dt) override {
        if (InputManager::getInstance().isKeyPressed(VK_RETURN)) {
            if (_heroWon && _run.allDefeated) {
                _onWin();
            } else {
                _onContinue();
            }
        }
    }
};

// ─── Win Screen ─────────────────────────────────────────────────────

class WinScreen : public Screen {
    RunState& _run;

public:
    WinScreen(RunState& run) : _run(run) {}

    void build() override {
        addEntity(makeWinBackdrop());

        float cx = (float)(WIDTH / 2);
        float cy = (float)(HEIGHT / 3);

        addEntity(new TextEntity(cx - 15, cy, "=== YOU CONQUERED THE GAUNTLET! ===", 93));
        addEntity(new TextEntity(cx - 12, cy + 3, "All monsters defeated!", 92));

        char buf[64];
        snprintf(buf, sizeof(buf), "Final Level: %d", _run.heroLevel);
        addEntity(new TextEntity(cx - 8, cy + 5, buf, 97));

        snprintf(buf, sizeof(buf), "Moves learned: %d", (int)_run.learnedMoves.size());
        addEntity(new TextEntity(cx - 8, cy + 7, buf, 97));

        addEntity(new TextEntity(cx - 12, cy + 10, "Press Q to exit. GG!", 90));
    }

    void update(float dt) override {
        // Q handled by engine
    }
};
