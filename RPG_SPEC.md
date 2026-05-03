# Turn-Based RPG — Design Spec

## Overview

A turn-based RPG built on the ANSIITY terminal game engine. The player controls a knight fighting through a gauntlet of 5 monsters, one at a time. Combat is turn-based: pick a move, the monster responds, repeat until someone drops to 0 HP. After defeating a monster, the hero learns one of its moves at random and can equip it for future fights. The player can replay previous encounters to farm XP or learn more moves.

A Flask server handles run configuration and monster AI. The client (ANSIITY engine) owns battle resolution, rendering, and state management.

Compile target resolution: **240×60** (default) or higher if sprites need more detail.

---

## Game States

```
Main Menu → Map → [Move Management] → Battle → Post-Battle → Map → ... → Win Screen
```

Each state owns its own entities. Transitioning between states destroys old entities and creates new ones. No scene file needed — entity setup is done in code per state.

| State | What's On Screen |
|---|---|
| **Main Menu** | Title, "Start Run", "Exit" |
| **Map** | 5 encounter nodes (horizontal or vertical), hero marker, current + previous encounters are selectable |
| **Move Management** | List of all learned moves, 4 equip slots, swap interface |
| **Battle** | Hero sprite (left), monster sprite (right), HP bars, move selection panel (4 buttons) |
| **Post-Battle (Win)** | "You learned [Move Name]!" display, continue button |
| **Post-Battle (Loss)** | "You lost!" display, return to map button |
| **Win Screen** | Victory message after clearing all 5 encounters |

---

## Server (Flask)

Two GET endpoints. No auth, no database, no persistence.

### `GET /run`

Called once when the player starts a new run. Returns the full run configuration.

Response:
```json
{
  "monsters": [
    {
      "name": "Goblin Warrior",
      "stats": { "health": 80, "attack": 12, "defense": 8, "magic": 4 },
      "moves": [
        { "name": "Rusty Blade", "type": "physical", "effect": "damage", "base_value": 15 },
        { "name": "Dirty Kick", "type": "physical", "effect": "damage_debuff", "base_value": 8, "debuff_stat": "defense", "debuff_turns": 2 },
        { "name": "Frenzy", "type": "none", "effect": "buff", "buff_stat": "attack", "buff_turns": 2 },
        { "name": "Headbutt", "type": "physical", "effect": "damage", "base_value": 25 }
      ]
    }
  ],
  "hero": {
    "base_stats": { "health": 100, "attack": 10, "defense": 10, "magic": 8 },
    "stat_growth": { "health": 15, "attack": 3, "defense": 2, "magic": 2 },
    "xp_per_level": 100,
    "moves": [
      { "name": "Slash", "type": "physical", "effect": "damage", "base_value": 15 },
      { "name": "Shield Up", "type": "none", "effect": "buff", "buff_stat": "defense", "buff_turns": 2 },
      { "name": "Battle Cry", "type": "none", "effect": "buff", "buff_stat": "attack", "buff_turns": 2 },
      { "name": "Second Wind", "type": "magic", "effect": "heal", "base_value": 20 }
    ]
  },
  "xp_rewards": [20, 35, 50, 70, 100]
}
```

### `GET /move`

Called each turn after the player acts. Client sends battle state as query params. Server returns the monster's chosen move.

Request: `GET /move?monster_id=0&monster_hp=65&hero_hp=80&monster_buffs=...&hero_buffs=...`

Response:
```json
{
  "move_index": 2
}
```

The server picks the move index from the monster's moveset. Bot logic lives here — can be random initially, smarter later (e.g. heal when low HP, buff when full HP).

---

## Battle System

### Turn Flow

1. Player selects one of 4 equipped moves
2. Client resolves player's move (damage/heal/buff applied immediately)
3. Check if monster is dead → if yes, go to Post-Battle (Win)
4. Client sends current battle state to `GET /move`
5. Server returns monster's move index
6. Client resolves monster's move
7. Check if hero is dead → if yes, go to Post-Battle (Loss)
8. Decrement all buff/debuff turn counters, remove expired ones
9. Next turn

### Damage Formulas

**Physical damage:**
```
raw = base_value * (attacker.effective_attack / 10)
damage = max(1, raw - target.effective_defense * 0.5)
```

**Magic damage:**
```
damage = base_value * (attacker.effective_magic / 10)
```
Magic bypasses defense entirely.

**Healing:**
```
heal = base_value * (caster.effective_magic / 10)
```

"Effective" stat = base stat + level growth + active buff/debuff modifiers.

These numbers are starting points — tune until fights feel right.

### Buff / Debuff System

Each buff/debuff is:
```
{ stat: "attack"|"defense"|"magic", modifier: +/- value, turns_remaining: int }
```

- Applied on move resolution
- Modify the *effective* stat, not the base stat
- Decremented at the end of each full turn (after both sides act)
- Removed when `turns_remaining` hits 0
- Buff modifier value: roughly 30–50% of the base stat (tune per move)

### Move Effects

| Effect | Description |
|---|---|
| `damage` | Deal damage (physical or magic scaling) |
| `heal` | Restore HP to self |
| `buff` | Raise one of own stats for N turns |
| `debuff` | Lower one of target's stats for N turns |
| `damage_debuff` | Deal damage AND apply a debuff |
| `damage_heal` | Deal damage AND heal self for the same amount (Drain Life) |
| `buff_self_damage` | Buff self but lose HP (Dark Pact) |

---

## Progression

### XP & Leveling

- Each monster awards XP on defeat (defined in run config, scales with encounter position)
- Flat threshold: 100 XP per level (configurable from server)
- On level up, all stats increase by `stat_growth` values from the run config
- Hero HP is fully restored on level up

### Move Learning

- On first defeat of a monster, the hero learns one random move from that monster's moveset
- On replay defeats, the hero learns another random move they don't already know (if any remain)
- All learned moves go into a pool
- The hero equips exactly 4 moves at a time
- Default knight moves are always available in the pool
- Move management screen: see full pool, drag/tap to swap into the 4 equip slots

---

## Map

- 5 encounter nodes displayed in order
- Current encounter is highlighted / selectable
- Previous encounters (already beaten) are selectable for replay
- Future encounters are visible but locked (greyed out)
- A "Manage Moves" button opens the move management screen
- Hero level and XP bar shown on the map

---

## Monsters (5 Encounters)

Encounter order (tuned for ascending difficulty):

| # | Monster | Archetype |
|---|---|---|
| 1 | Goblin Warrior | Physical bruiser, straightforward |
| 2 | Giant Spider | Physical + debuffs |
| 3 | Goblin Mage | Magic damage + self-buffs |
| 4 | Witch | Magic + drain + curse |
| 5 | Dragon | Mixed physical/magic, boss-tier stats |

Monster stats scale with position — monster 1 is beatable at level 1, monster 5 requires leveling.

---

## Sprites

All characters rendered as `.ansii` sprites. Format: `WxH` header, then rows of `<colorcode><char>` tokens.

- Hero sprite: left side of battle screen
- Monster sprite: right side of battle screen
- Sprites should be large enough to read (roughly 15–30 cells wide, 15–25 cells tall)
- Color code 0 = transparent

Pixel art assets will be converted to `.ansii` format via a Python converter script.

---

## Client Data Model

```
RunState:
  monsters[]          — from GET /run
  hero_base_stats     — from GET /run
  hero_level          — starts at 1
  hero_xp             — starts at 0
  learned_moves[]     — starts with default knight moves
  equipped_moves[4]   — starts with default knight moves
  current_encounter   — 0..4
  defeated[]          — which encounters have been beaten

BattleState:
  hero_hp
  monster_hp
  hero_buffs[]
  monster_buffs[]
  turn_number

Buff:
  stat: string
  modifier: float
  turns_remaining: int
```

---

## File Structure

```
game/
  rpg/
    rpg_state.h          — RunState, BattleState structs
    menu_entity.h        — Main menu screen
    map_entity.h         — Map / encounter selection
    battle_entity.h      — Battle screen, turn logic, move resolution
    hud_entity.h         — HP bars, move buttons, status display
    move_mgmt_entity.h   — Move swap screen
    http_client.h        — Minimal HTTP GET for Flask endpoints
main_rpg.cpp             — Entry point, factory registration
levels/
  rpg.scene              — (optional, may not need one)
sprites/
  knight.ansii
  goblin_warrior.ansii
  giant_spider.ansii
  goblin_mage.ansii
  witch.ansii
  dragon.ansii
server/
  app.py                 — Flask server
  config.py              — Monster/hero definitions
```
