#pragma once

#include "screen.h"
#include "rpg_state.h"
#include "http_client.h"
#include "char_select_screen.h"  // for buildMoveInfo
#include "backdrop.h"
#include "../../engine/constants.h"
#include <functional>
#include <deque>
#include <cmath>
#include <algorithm>

// ─── Animation queue entry ──────────────────────────────────────────

struct BattleAnim {
    enum Type {
        WAIT,           // pause for duration
        DAMAGE_NUMBER,  // floating number
        FLASH_HIT,      // flash overlay on target
        SHAKE,          // shake a sprite
        TEXT_POP,       // text popup (move name, "MISS", etc.)
        REBUILD_UI,     // refresh the static UI after state changes
    };

    Type type;
    float duration = 0;
    std::string text;
    int color = 97;
    bool targetIsHero = false;  // which side the effect targets
    int value = 0;              // damage/heal amount
};

// ─── Battle Screen ──────────────────────────────────────────────────

class BattleScreen : public Screen {
    RunState& _run;
    BattleState _battle;
    HttpClient& _http;
    std::function<void(bool, BattleState&)> _onEnd; // (heroWon, battleState)

    int _selected = 0;
    bool _acceptingInput = true;

    // Animation
    std::deque<BattleAnim> _animQueue;
    float _animTimer = 0;
    BattleAnim _currentAnim;
    bool _animPlaying = false;
    std::vector<Entity*> _animEntities; // transient anim entities (separate from _entities)

    // Sprite references for shake effects
    Entity* _heroSprite = nullptr;
    Entity* _monsterSprite = nullptr;
    float _heroBaseX = 15;
    float _monsterBaseX = 0;

    // Layout constants
    static constexpr float HERO_X = 15;
    static constexpr float SPRITE_Y = 8;
    static constexpr float PANEL_Y_OFFSET = 15; // from bottom

public:
    BattleScreen(RunState& run, HttpClient& http, int encounterIdx,
                 std::function<void(bool, BattleState&)> onEnd)
        : _run(run), _http(http), _onEnd(std::move(onEnd)) {
        _battle = {};
        _battle.monsterIdx = encounterIdx;
        _battle.heroHP = run.heroStats.health;
        _battle.heroMaxHP = run.heroStats.health;
        _battle.monsterHP = run.monsters[encounterIdx].stats.health;
        _battle.monsterMaxHP = run.monsters[encounterIdx].stats.health;
        _battle.heroStats = run.heroStats;
        _battle.monsterStats = run.monsters[encounterIdx].stats;
        _battle.turnNumber = 1;
        _monsterBaseX = (float)(WIDTH - 50);
    }

    void build() override {
        auto& monster = _run.monsters[_battle.monsterIdx];

        // Environmental backdrop
        addEntity(new BackdropEntity(_battle.monsterIdx));

        // Hero sprite
        _heroSprite = addEntity(new SpriteEntity(HERO_X, SPRITE_Y, _run.heroDef.spriteRight, 10));
        _heroBaseX = HERO_X;

        // Monster sprite
        _monsterSprite = addEntity(new SpriteEntity(_monsterBaseX, SPRITE_Y, monster.sprite, 10));

        buildUI();
    }

    void update(float dt) override {
        // Play animations before accepting input
        if (_animPlaying) {
            updateAnimation(dt);
            return;
        }

        if (!_animQueue.empty()) {
            startNextAnim();
            return;
        }

        if (_battle.battleOver) return;
        if (!_acceptingInput) return;

        updateInput();
    }

private:
    // ─── Static UI (rebuilt after each turn) ────────────────────────

    // Track UI entities separately so we can rebuild just the UI part
    std::vector<Entity*> _uiEntities;

    void clearUI() {
        for (auto* e : _uiEntities) {
            e->destroy();
            // Remove from _entities to avoid double-destroy in cleanup()
            _entities.erase(
                std::remove(_entities.begin(), _entities.end(), e),
                _entities.end());
        }
        _uiEntities.clear();
    }

    Entity* addUI(Entity* e) {
        e->init();
        _uiEntities.push_back(e);
        _entities.push_back(e); // also track in screen entities for full cleanup
        return e;
    }

    void buildUI() {
        clearUI();
        auto& monster = _run.monsters[_battle.monsterIdx];
        float panelY = (float)(HEIGHT - PANEL_Y_OFFSET);

        // Names
        addUI(new TextEntity(HERO_X, 5, _run.heroDef.name, 97));
        addUI(new TextEntity(_monsterBaseX, 5, monster.name, 91));

        // HP bars
        addUI(new HPBarEntity(HERO_X, 6, _battle.heroMaxHP, &_battle.heroHP, 30, 32));
        addUI(new HPBarEntity(_monsterBaseX, 6, _battle.monsterMaxHP, &_battle.monsterHP, 30, 31));

        // HP text
        char hpBuf[64];
        snprintf(hpBuf, sizeof(hpBuf), "HP: %d/%d", _battle.heroHP, _battle.heroMaxHP);
        addUI(new TextEntity(HERO_X, 7, hpBuf, 32));
        snprintf(hpBuf, sizeof(hpBuf), "HP: %d/%d", _battle.monsterHP, _battle.monsterMaxHP);
        addUI(new TextEntity(_monsterBaseX, 7, hpBuf, 31));

        // Turn indicator
        char turnBuf[32];
        snprintf(turnBuf, sizeof(turnBuf), "Turn %d", _battle.turnNumber);
        addUI(new TextEntity((float)(WIDTH / 2 - 4), panelY - 4, turnBuf, 90));

        // ── Separator line between scene and UI ──
        std::string sepLine(WIDTH - 4, '-');
        addUI(new TextEntity(2, panelY - 3, sepLine, 90));

        // Move selection
        if (_acceptingInput && !_battle.battleOver) {
            addUI(new TextEntity(5, panelY - 2, "=== YOUR TURN - Select a move ===", 96));

            // Draw move buttons with highlight box on selected
            for (int i = 0; i < 4; i++) {
                char label[64];
                snprintf(label, sizeof(label), " [%d] %s ", i + 1, _run.equippedMoves[i].name.c_str());
                int labelLen = (int)strlen(label);
                float mx = 3 + (float)(i * 30);

                if (i == _selected) {
                    // Draw highlight box around selected move
                    std::string top = "+" + std::string(labelLen, '-') + "+";
                    std::string bot = "+" + std::string(labelLen, '-') + "+";
                    addUI(new TextEntity(mx, panelY - 1, top, 93));
                    addUI(new TextEntity(mx, panelY, "|", 93));
                    addUI(new TextEntity(mx + 1, panelY, label, 93));
                    addUI(new TextEntity(mx + 1 + (float)labelLen, panelY, "|", 93));
                    addUI(new TextEntity(mx, panelY + 1, bot, 93));
                } else {
                    addUI(new TextEntity(mx + 1, panelY, label, 37));
                }
            }

            // Move info for currently highlighted move (right side)
            auto& selMove = _run.equippedMoves[_selected];
            float infoX = (float)(WIDTH - 45);
            addUI(new TextEntity(infoX, panelY - 2, selMove.name, 93));
            buildMoveInfoWith([this](Entity* e) { addUI(e); }, infoX + 2, panelY - 1, selMove);
        }

        // Battle log
        if (!_battle.lastHeroMove.empty()) {
            addUI(new TextEntity(5, panelY + 4,
                "You: " + _battle.lastHeroMove + " - " + _battle.lastHeroResult, 32));
        }
        if (!_battle.lastMonsterMove.empty()) {
            addUI(new TextEntity(5, panelY + 5,
                monster.name + ": " + _battle.lastMonsterMove + " - " + _battle.lastMonsterResult, 31));
        }

        // Buff indicators
        float buffY = panelY + 7;
        if (!_battle.heroBuffs.empty()) {
            std::string bufStr = "Your buffs: ";
            for (auto& b : _battle.heroBuffs) {
                char bb[32];
                snprintf(bb, sizeof(bb), "%s%+d(%dt) ", b.stat.c_str(), b.modifier, b.turnsRemaining);
                bufStr += bb;
            }
            addUI(new TextEntity(5, buffY, bufStr, 36));
        }
        if (!_battle.monsterBuffs.empty()) {
            std::string bufStr = "Enemy buffs: ";
            for (auto& b : _battle.monsterBuffs) {
                char bb[32];
                snprintf(bb, sizeof(bb), "%s%+d(%dt) ", b.stat.c_str(), b.modifier, b.turnsRemaining);
                bufStr += bb;
            }
            addUI(new TextEntity(5, buffY + 1, bufStr, 35));
        }
    }

    // ─── Input handling ─────────────────────────────────────────────

    void updateInput() {
        auto& input = InputManager::getInstance();

        bool needRebuild = false;
        if (input.isKeyPressed(VK_LEFT) && _selected > 0) { _selected--; needRebuild = true; }
        if (input.isKeyPressed(VK_RIGHT) && _selected < 3) { _selected++; needRebuild = true; }

        int moveIdx = -1;
        if (input.isKeyPressed('1')) moveIdx = 0;
        if (input.isKeyPressed('2')) moveIdx = 1;
        if (input.isKeyPressed('3')) moveIdx = 2;
        if (input.isKeyPressed('4')) moveIdx = 3;
        if (input.isKeyPressed(VK_RETURN)) moveIdx = _selected;

        if (moveIdx >= 0 && moveIdx < 4) {
            executeTurn(moveIdx);
            return;
        }

        if (needRebuild) buildUI();
    }

    // ─── Turn execution ─────────────────────────────────────────────

    void executeTurn(int moveIdx) {
        _acceptingInput = false;
        auto& monster = _run.monsters[_battle.monsterIdx];

        // ── Hero's move ──
        Move& move = _run.equippedMoves[moveIdx];
        _battle.lastHeroMove = move.name;

        // Queue: show move name
        queueAnim(BattleAnim::TEXT_POP, 0.6f, move.name, 93, false);

        // Resolve
        _battle.lastHeroResult = resolveMove(move,
            _battle.heroStats, _battle.heroHP,
            _battle.monsterStats, _battle.monsterHP,
            _battle.heroBuffs, _battle.monsterBuffs, _battle);

        // Queue: hit effects on monster
        if (move.effect == "damage" || move.effect == "damage_debuff" || move.effect == "damage_heal") {
            queueAnim(BattleAnim::FLASH_HIT, 0.15f, "", 91, false); // flash on monster
            queueAnim(BattleAnim::SHAKE, 0.3f, "", 0, false);       // shake monster
            queueAnim(BattleAnim::DAMAGE_NUMBER, 0.8f, _battle.lastHeroResult, 91, false);
        } else if (move.effect == "heal") {
            queueAnim(BattleAnim::DAMAGE_NUMBER, 0.8f, _battle.lastHeroResult, 92, true); // green on hero
        } else if (move.effect == "buff") {
            queueAnim(BattleAnim::DAMAGE_NUMBER, 0.8f, _battle.lastHeroResult, 96, true);
        } else if (move.effect == "debuff") {
            queueAnim(BattleAnim::DAMAGE_NUMBER, 0.8f, _battle.lastHeroResult, 95, false);
        } else if (move.effect == "buff_self_damage") {
            queueAnim(BattleAnim::DAMAGE_NUMBER, 0.8f, _battle.lastHeroResult, 93, true);
        }

        queueAnim(BattleAnim::REBUILD_UI, 0, "", 0, false);

        // Check monster death
        if (_battle.monsterHP <= 0) {
            queueAnim(BattleAnim::TEXT_POP, 1.0f, "DEFEATED!", 92, false);
            queueAnim(BattleAnim::WAIT, 0.3f, "", 0, false);
            _battle.battleOver = true;
            _battle.heroWon = true;
            // onEnd called after anims play out
            return;
        }

        // ── Monster's move (fetch from server) ──
        queueAnim(BattleAnim::WAIT, 0.3f, "", 0, false);

        char url[256];
        snprintf(url, sizeof(url), "/move?monster_id=%d&monster_hp=%d&monster_max_hp=%d&hero_hp=%d",
            _battle.monsterIdx, _battle.monsterHP, _battle.monsterMaxHP, _battle.heroHP);
        std::string response = _http.get(url);
        int mMoveIdx = json::getInt(response, "move_index");

        if (mMoveIdx >= 0 && mMoveIdx < (int)monster.moves.size()) {
            Move& mMove = monster.moves[mMoveIdx];
            _battle.lastMonsterMove = mMove.name;

            // Queue: show monster move name
            queueAnim(BattleAnim::TEXT_POP, 0.6f, mMove.name, 91, true);

            _battle.lastMonsterResult = resolveMove(mMove,
                _battle.monsterStats, _battle.monsterHP,
                _battle.heroStats, _battle.heroHP,
                _battle.monsterBuffs, _battle.heroBuffs, _battle);

            // Queue: hit effects on hero
            if (mMove.effect == "damage" || mMove.effect == "damage_debuff" || mMove.effect == "damage_heal") {
                queueAnim(BattleAnim::FLASH_HIT, 0.15f, "", 91, true);
                queueAnim(BattleAnim::SHAKE, 0.3f, "", 0, true);
                queueAnim(BattleAnim::DAMAGE_NUMBER, 0.8f, _battle.lastMonsterResult, 91, true);
            } else if (mMove.effect == "heal") {
                queueAnim(BattleAnim::DAMAGE_NUMBER, 0.8f, _battle.lastMonsterResult, 92, false);
            } else if (mMove.effect == "buff") {
                queueAnim(BattleAnim::DAMAGE_NUMBER, 0.8f, _battle.lastMonsterResult, 96, false);
            } else if (mMove.effect == "debuff") {
                queueAnim(BattleAnim::DAMAGE_NUMBER, 0.8f, _battle.lastMonsterResult, 95, true);
            } else if (mMove.effect == "buff_self_damage") {
                queueAnim(BattleAnim::DAMAGE_NUMBER, 0.8f, _battle.lastMonsterResult, 93, false);
            }

            queueAnim(BattleAnim::REBUILD_UI, 0, "", 0, false);
        }

        // Check hero death
        if (_battle.heroHP <= 0) {
            queueAnim(BattleAnim::TEXT_POP, 1.0f, "YOU FELL...", 91, true);
            queueAnim(BattleAnim::WAIT, 0.3f, "", 0, false);
            _battle.battleOver = true;
            _battle.heroWon = false;
            return;
        }

        // Tick buffs and advance turn
        _battle.tickBuffs();
        _battle.turnNumber++;

        // After all anims: re-enable input
        // (handled in startNextAnim when queue empties)
    }

    // ─── Animation playback ─────────────────────────────────────────

    void queueAnim(BattleAnim::Type type, float duration, const std::string& text,
                   int color, bool targetHero) {
        BattleAnim a;
        a.type = type;
        a.duration = duration;
        a.text = text;
        a.color = color;
        a.targetIsHero = targetHero;
        _animQueue.push_back(a);
    }

    void startNextAnim() {
        if (_animQueue.empty()) {
            // All animations done
            _animPlaying = false;
            if (_battle.battleOver) {
                _onEnd(_battle.heroWon, _battle);
            } else {
                _acceptingInput = true;
                _selected = 0;
                buildUI();
            }
            return;
        }

        _currentAnim = _animQueue.front();
        _animQueue.pop_front();
        _animTimer = 0;
        _animPlaying = true;

        // Clean up previous anim entities
        for (auto* e : _animEntities) e->destroy();
        _animEntities.clear();

        float targetX = _currentAnim.targetIsHero ? _heroBaseX : _monsterBaseX;
        float targetY = SPRITE_Y;

        switch (_currentAnim.type) {
            case BattleAnim::WAIT:
                // Just wait
                break;

            case BattleAnim::DAMAGE_NUMBER: {
                // Float upward from target
                auto* ent = new AnimEntity(
                    targetX + 5, targetY - 1,
                    _currentAnim.text, _currentAnim.color,
                    _currentAnim.duration, 0, -3.0f, 90);
                ent->init();
                _animEntities.push_back(ent);
                break;
            }

            case BattleAnim::FLASH_HIT: {
                // Brief colored flash over target sprite area
                auto* ent = new FlashEntity(
                    targetX, targetY, 30, 16,
                    _currentAnim.color, _currentAnim.duration, 75);
                ent->init();
                _animEntities.push_back(ent);
                break;
            }

            case BattleAnim::SHAKE:
                // Handled in updateAnimation — oscillate sprite position
                break;

            case BattleAnim::TEXT_POP: {
                // Show move/status name centered above the action
                float cx = (float)(WIDTH / 2 - (int)_currentAnim.text.size() / 2);
                auto* ent = new AnimEntity(
                    cx, (float)(HEIGHT / 2 - 2),
                    _currentAnim.text, _currentAnim.color,
                    _currentAnim.duration, 0, 0, 90);
                ent->init();
                _animEntities.push_back(ent);
                break;
            }

            case BattleAnim::REBUILD_UI:
                buildUI();
                _animPlaying = false;
                startNextAnim(); // instant — chain to next (or re-enable input if queue empty)
                return;
        }
    }

    void updateAnimation(float dt) {
        _animTimer += dt;

        // Shake effect: oscillate sprite position
        if (_currentAnim.type == BattleAnim::SHAKE) {
            Entity* target = _currentAnim.targetIsHero ? _heroSprite : _monsterSprite;
            float baseX = _currentAnim.targetIsHero ? _heroBaseX : _monsterBaseX;
            if (target && target->renderer) {
                float intensity = 2.0f * (1.0f - _animTimer / _currentAnim.duration); // fade out
                float offset = std::sin(_animTimer * 40.0f) * intensity;
                target->renderer->x = baseX + offset;
            }
        }

        if (_animTimer >= _currentAnim.duration) {
            // Restore shake position
            if (_currentAnim.type == BattleAnim::SHAKE) {
                Entity* target = _currentAnim.targetIsHero ? _heroSprite : _monsterSprite;
                float baseX = _currentAnim.targetIsHero ? _heroBaseX : _monsterBaseX;
                if (target && target->renderer) {
                    target->renderer->x = baseX;
                }
            }

            // Clean up anim entities
            for (auto* e : _animEntities) e->destroy();
            _animEntities.clear();

            _animPlaying = false;
            startNextAnim();
        }
    }
};
