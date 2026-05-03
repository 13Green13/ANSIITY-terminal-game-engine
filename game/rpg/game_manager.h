#pragma once

#include "../../engine/engine.h"
#include "../../engine/input_manager.h"
#include "rpg_state.h"
#include "http_client.h"
#include "screen.h"
#include "menu_screen.h"
#include "char_select_screen.h"
#include "map_screen.h"
#include "battle_screen.h"
#include "move_mgmt_screen.h"
#include "post_battle_screen.h"
#include <memory>
#include <cstdlib>
#include <ctime>

// ─── Game Manager (thin orchestrator) ───────────────────────────────

class GameManager : public Entity {
    RunState _run;
    HttpClient _http;
    std::unique_ptr<Screen> _screen;

public:
    GameManager() : _http("127.0.0.1", 5000) {
        srand((unsigned)time(nullptr));

        auto& input = InputManager::getInstance();
        input.watchKey(VK_UP);
        input.watchKey(VK_DOWN);
        input.watchKey(VK_LEFT);
        input.watchKey(VK_RIGHT);
        input.watchKey(VK_RETURN);
        input.watchKey(VK_ESCAPE);
        input.watchKey('1');
        input.watchKey('2');
        input.watchKey('3');
        input.watchKey('4');
        input.watchKey('M');
    }

    void update(float dt) override {
        if (_screen) _screen->update(dt);
    }

    // Called once after init to show menu
    void showMenu() {
        switchTo(makeMenu());
    }

private:
    void switchTo(std::unique_ptr<Screen> next) {
        if (_screen) _screen->cleanup();
        _screen = std::move(next);
        _screen->build();
    }

    // ─── Screen factories ───────────────────────────────────────────

    std::unique_ptr<Screen> makeMenu() {
        return std::make_unique<MenuScreen>(_run, _http,
            [this](int choice) {
                if (choice == 0) switchTo(makeCharSelect());
                else Engine::getInstance().stop();
            });
    }

    std::unique_ptr<Screen> makeCharSelect() {
        return std::make_unique<CharSelectScreen>(_run, _http,
            [this]() { switchTo(makeMap()); },
            [this]() { switchTo(makeMenu()); });
    }

    std::unique_ptr<Screen> makeMap() {
        return std::make_unique<MapScreen>(_run,
            [this](int idx) { switchTo(makeBattle(idx)); },
            [this]() { switchTo(makeMoveMgmt()); });
    }

    std::unique_ptr<Screen> makeBattle(int encounterIdx) {
        return std::make_unique<BattleScreen>(_run, _http, encounterIdx,
            [this](bool heroWon, BattleState& battle) {
                onBattleEnd(heroWon, battle);
            });
    }

    std::unique_ptr<Screen> makeMoveMgmt() {
        return std::make_unique<MoveMgmtScreen>(_run,
            [this]() { switchTo(makeMap()); });
    }

    std::unique_ptr<Screen> makePostBattle(bool heroWon, int monsterIdx,
                                            const std::string& learnedMove) {
        return std::make_unique<PostBattleScreen>(_run, heroWon, monsterIdx, learnedMove,
            [this]() { switchTo(makeMap()); },
            [this]() { switchTo(makeWin()); });
    }

    std::unique_ptr<Screen> makeWin() {
        return std::make_unique<WinScreen>(_run);
    }

    // ─── Battle end logic (awards XP, learns moves, advances map) ──

    void onBattleEnd(bool heroWon, BattleState& battle) {
        std::string learnedMoveName;

        if (heroWon) {
            int idx = battle.monsterIdx;
            if (idx < (int)_run.xpRewards.size()) {
                _run.addXP(_run.xpRewards[idx]);
            }
            learnedMoveName = _run.learnRandomMove(idx);
            if (!_run.defeated[idx]) {
                _run.defeated[idx] = true;
                if (_run.currentEncounter == idx && idx < (int)_run.monsters.size() - 1) {
                    _run.currentEncounter = idx + 1;
                }
            }
            bool allDone = true;
            for (int i = 0; i < (int)_run.defeated.size(); i++) {
                if (!_run.defeated[i]) { allDone = false; break; }
            }
            _run.allDefeated = allDone;
        }

        switchTo(makePostBattle(heroWon, battle.monsterIdx, learnedMoveName));
    }
};
