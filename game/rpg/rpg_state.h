#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include "json_parse.h"
#include "http_client.h"

// ─── Data Types ─────────────────────────────────────────────────────

struct Stats {
    int health = 100;
    int attack = 10;
    int defense = 10;
    int magic = 8;
};

struct Move {
    std::string name;
    std::string type;       // "physical", "magic", "none"
    std::string effect;     // "damage", "heal", "buff", "debuff", "damage_debuff", "damage_heal", "buff_self_damage"
    int baseValue = 0;
    std::string buffStat;   // which stat to buff/debuff
    int buffAmount = 0;
    int buffTurns = 0;
    std::string debuffStat;
    int debuffAmount = 0;
    int debuffTurns = 0;
    int selfDamage = 0;     // for buff_self_damage
};

struct MonsterDef {
    std::string name;
    std::string sprite;
    Stats stats;
    std::vector<Move> moves;
};

struct HeroDef {
    std::string name;
    std::string sprite;       // left-facing (for char select)
    std::string spriteRight;  // right-facing (for battle)
    Stats baseStats;
    Stats statGrowth;
    int xpPerLevel = 100;
    std::vector<Move> moves;
};

struct Buff {
    std::string stat;
    int modifier = 0;
    int turnsRemaining = 0;
};

// ─── Game State ─────────────────────────────────────────────────────

enum class GameScreen {
    MENU,
    MAP,
    MOVE_MGMT,
    BATTLE,
    POST_BATTLE,
    WIN_SCREEN,
};

struct RunState {
    std::vector<MonsterDef> monsters;
    std::vector<HeroDef> heroChoices; // all 4 hero options
    HeroDef heroDef;
    std::vector<int> xpRewards;

    int heroLevel = 1;
    int heroXP = 0;
    Stats heroStats;             // current effective base stats (base + level growth)
    std::vector<Move> learnedMoves;
    Move equippedMoves[4];
    int currentEncounter = 0;
    std::vector<bool> defeated;  // which encounters have been beaten at least once
    bool allDefeated = false;

    void init() {
        heroLevel = 1;
        heroXP = 0;
        heroStats = heroDef.baseStats;
        learnedMoves = heroDef.moves;
        for (int i = 0; i < 4 && i < (int)heroDef.moves.size(); i++) {
            equippedMoves[i] = heroDef.moves[i];
        }
        currentEncounter = 0;
        defeated.assign(monsters.size(), false);
        allDefeated = false;
    }

    void addXP(int xp) {
        heroXP += xp;
        while (heroXP >= heroDef.xpPerLevel) {
            heroXP -= heroDef.xpPerLevel;
            heroLevel++;
            heroStats.health += heroDef.statGrowth.health;
            heroStats.attack += heroDef.statGrowth.attack;
            heroStats.defense += heroDef.statGrowth.defense;
            heroStats.magic += heroDef.statGrowth.magic;
        }
    }

    // Learn a random move from a monster that we don't already know
    // Returns the learned move name, or "" if nothing new
    std::string learnRandomMove(int monsterIdx) {
        auto& mMoves = monsters[monsterIdx].moves;
        std::vector<int> candidates;
        for (int i = 0; i < (int)mMoves.size(); i++) {
            bool known = false;
            for (auto& lm : learnedMoves) {
                if (lm.name == mMoves[i].name) { known = true; break; }
            }
            if (!known) candidates.push_back(i);
        }
        if (candidates.empty()) return "";
        int pick = candidates[rand() % candidates.size()];
        learnedMoves.push_back(mMoves[pick]);
        return mMoves[pick].name;
    }
};

struct BattleState {
    int heroHP = 0;
    int heroMaxHP = 0;
    int monsterHP = 0;
    int monsterMaxHP = 0;
    int monsterIdx = 0;
    int turnNumber = 0;
    Stats heroStats;         // copy of hero stats for this battle
    Stats monsterStats;      // copy of monster stats for this battle
    std::vector<Buff> heroBuffs;
    std::vector<Buff> monsterBuffs;
    bool battleOver = false;
    bool heroWon = false;
    std::string lastHeroMove;
    std::string lastMonsterMove;
    std::string lastHeroResult;
    std::string lastMonsterResult;

    int getEffectiveStat(const Stats& base, const std::vector<Buff>& buffs, const std::string& stat) const {
        int val = 0;
        if (stat == "attack") val = base.attack;
        else if (stat == "defense") val = base.defense;
        else if (stat == "magic") val = base.magic;
        else if (stat == "health") val = base.health;
        for (auto& b : buffs) {
            if (b.stat == stat) val += b.modifier;
        }
        return std::max(0, val);
    }

    void tickBuffs() {
        auto tick = [](std::vector<Buff>& buffs) {
            for (auto& b : buffs) b.turnsRemaining--;
            buffs.erase(std::remove_if(buffs.begin(), buffs.end(),
                [](const Buff& b) { return b.turnsRemaining <= 0; }), buffs.end());
        };
        tick(heroBuffs);
        tick(monsterBuffs);
    }
};

// ─── Move Resolution ────────────────────────────────────────────────

inline std::string resolveMove(const Move& move,
                               Stats& attackerStats, int& attackerHP,
                               Stats& targetStats, int& targetHP,
                               std::vector<Buff>& attackerBuffs,
                               std::vector<Buff>& targetBuffs,
                               const BattleState& battle) {
    int effAtk = battle.getEffectiveStat(attackerStats, attackerBuffs, "attack");
    int effMag = battle.getEffectiveStat(attackerStats, attackerBuffs, "magic");
    int effDef = battle.getEffectiveStat(targetStats, targetBuffs, "defense");

    std::string result;
    int damage = 0;

    auto calcDamage = [&]() {
        if (move.type == "physical") {
            float raw = move.baseValue * (effAtk / 10.0f);
            damage = std::max(1, (int)(raw - effDef * 0.5f));
        } else if (move.type == "magic") {
            damage = std::max(1, (int)(move.baseValue * (effMag / 10.0f)));
        }
    };

    if (move.effect == "damage") {
        calcDamage();
        targetHP = std::max(0, targetHP - damage);
        result = std::to_string(damage) + " damage";
    }
    else if (move.effect == "heal") {
        int heal = std::max(1, (int)(move.baseValue * (effMag / 10.0f)));
        int maxHP = attackerStats.health; // base max
        attackerHP = std::min(maxHP, attackerHP + heal);
        result = "healed " + std::to_string(heal);
    }
    else if (move.effect == "buff") {
        attackerBuffs.push_back({move.buffStat, move.buffAmount, move.buffTurns});
        result = "+" + std::to_string(move.buffAmount) + " " + move.buffStat;
    }
    else if (move.effect == "debuff") {
        targetBuffs.push_back({move.debuffStat, -move.debuffAmount, move.debuffTurns});
        result = "-" + std::to_string(move.debuffAmount) + " " + move.debuffStat + " (target)";
    }
    else if (move.effect == "damage_debuff") {
        calcDamage();
        targetHP = std::max(0, targetHP - damage);
        targetBuffs.push_back({move.debuffStat, -move.debuffAmount, move.debuffTurns});
        result = std::to_string(damage) + " dmg, -" + std::to_string(move.debuffAmount) + " " + move.debuffStat;
    }
    else if (move.effect == "damage_heal") {
        calcDamage();
        targetHP = std::max(0, targetHP - damage);
        int maxHP = attackerStats.health;
        attackerHP = std::min(maxHP, attackerHP + damage);
        result = std::to_string(damage) + " dmg, healed " + std::to_string(damage);
    }
    else if (move.effect == "buff_self_damage") {
        attackerBuffs.push_back({move.buffStat, move.buffAmount, move.buffTurns});
        attackerHP = std::max(0, attackerHP - move.selfDamage);
        result = "+" + std::to_string(move.buffAmount) + " " + move.buffStat + ", lost " + std::to_string(move.selfDamage) + " HP";
    }

    return result;
}

// ─── Parse server response ──────────────────────────────────────────

inline Move parseMove(const std::string& obj) {
    Move m;
    m.name = json::getString(obj, "name");
    m.type = json::getString(obj, "type");
    m.effect = json::getString(obj, "effect");
    m.baseValue = json::getInt(obj, "base_value");
    m.buffStat = json::getString(obj, "buff_stat");
    m.buffAmount = json::getInt(obj, "buff_amount");
    m.buffTurns = json::getInt(obj, "buff_turns");
    m.debuffStat = json::getString(obj, "debuff_stat");
    m.debuffAmount = json::getInt(obj, "debuff_amount");
    m.debuffTurns = json::getInt(obj, "debuff_turns");
    m.selfDamage = json::getInt(obj, "self_damage");
    return m;
}

inline MonsterDef parseMonster(const std::string& obj) {
    MonsterDef m;
    m.name = json::getString(obj, "name");
    m.sprite = json::getString(obj, "sprite");
    std::string statsObj = json::getObject(obj, obj.find("\"stats\""));
    m.stats.health = json::getInt(statsObj, "health");
    m.stats.attack = json::getInt(statsObj, "attack");
    m.stats.defense = json::getInt(statsObj, "defense");
    m.stats.magic = json::getInt(statsObj, "magic");
    std::string movesArr = json::getArray(obj, "moves");
    for (auto& mo : json::splitArrayObjects(movesArr)) {
        m.moves.push_back(parseMove(mo));
    }
    return m;
}

inline HeroDef parseHero(const std::string& heroObj) {
    HeroDef def;
    def.name = json::getString(heroObj, "name");
    def.sprite = json::getString(heroObj, "sprite");
    def.spriteRight = json::getString(heroObj, "sprite_right");

    std::string baseStatsObj = json::getObject(heroObj, heroObj.find("\"base_stats\""));
    def.baseStats.health = json::getInt(baseStatsObj, "health");
    def.baseStats.attack = json::getInt(baseStatsObj, "attack");
    def.baseStats.defense = json::getInt(baseStatsObj, "defense");
    def.baseStats.magic = json::getInt(baseStatsObj, "magic");

    std::string growthObj = json::getObject(heroObj, heroObj.find("\"stat_growth\""));
    def.statGrowth.health = json::getInt(growthObj, "health");
    def.statGrowth.attack = json::getInt(growthObj, "attack");
    def.statGrowth.defense = json::getInt(growthObj, "defense");
    def.statGrowth.magic = json::getInt(growthObj, "magic");

    def.xpPerLevel = json::getInt(heroObj, "xp_per_level");

    std::string heroMovesArr = json::getArray(heroObj, "moves");
    for (auto& mo : json::splitArrayObjects(heroMovesArr)) {
        def.moves.push_back(parseMove(mo));
    }
    return def;
}

inline bool parseRunConfig(const std::string& responseBody, RunState& run) {
    if (responseBody.empty()) return false;

    // Parse monsters
    std::string monstersArr = json::getArray(responseBody, "monsters");
    for (auto& mo : json::splitArrayObjects(monstersArr)) {
        run.monsters.push_back(parseMonster(mo));
    }

    // Parse all hero choices
    std::string heroesArr = json::getArray(responseBody, "heroes");
    if (!heroesArr.empty()) {
        for (auto& ho : json::splitArrayObjects(heroesArr)) {
            run.heroChoices.push_back(parseHero(ho));
        }
    }

    // Parse selected hero
    std::string heroObj = json::getObject(responseBody, responseBody.find("\"hero\""));
    run.heroDef = parseHero(heroObj);

    // Parse XP rewards
    std::string xpArr = json::getArray(responseBody, "xp_rewards");
    run.xpRewards = json::splitArrayInts(xpArr);

    run.init();
    return true;
}
