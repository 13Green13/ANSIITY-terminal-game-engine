# ANSIITY RPG

A turn-based RPG built on the ANSIITY terminal game engine. The player controls a hero fighting through a gauntlet of monsters in a Powershell console rendered at 240×60 cells using 16 ANSI colors.

## How to Play

Double-click `play.bat`. This starts the backend server and launches the game automatically. No installs needed.

To edit game configuration (monsters, heroes, stats, moves), run `rpg_server.exe` separately and open `http://127.0.0.1:5000/` in a browser.

### Controls

| Screen | Key | Action |
|--------|-----|--------|
| Menu | ↑/↓, Enter | Navigate, select |
| Character Select | ←/→, Enter, Esc | Browse heroes, confirm, back |
| Map | ←/→, Enter, 1-5 | Select encounter, enter battle |
| Map | M | Open move management |
| Battle | ←/→, 1-4, Enter | Select and confirm move |
| Move Management | ↑/↓, 1-4, Enter, Esc | Navigate, pick slot, swap, back |
| Any | Q | Quit |

---

## Spec Coverage

### Core Requirements

| Requirement | Status | Details |
|-------------|--------|---------|
| **Main Menu** | ✅ | Start New Run / Exit. Castle entrance backdrop. |
| **Map / Run Overview** | ✅ | All encounters shown with status (Current/Defeated/Locked/Replay). Hero stats displayed. Parchment map backdrop. |
| **Enter battle from map** | ✅ | Click (number keys or arrow+enter) any accessible encounter. |
| **View equipped moves** | ✅ | Shown on map screen and in move management. |
| **Move management screen** | ✅ | Swap any learned move into the 4 equip slots. Info panels show full move details. |
| **Battle screen** | ✅ | Hero and monster sprites, HP bars, HP text, turn counter, move selection with highlight box, battle log, buff indicators. Separator line between scene and UI. |
| **Post-battle (loss)** | ✅ | Player returns to map, can retry. |
| **Post-battle (win)** | ✅ | XP awarded, level-up applied, random monster move learned, shown to player. |
| **Replay fights** | ✅ | Defeated encounters remain accessible for XP grinding and move learning. |
| **Progression** | ✅ | Hero starts at Level 1. XP per monster defined in config. Level-up increases all stats by class-specific growth values. |
| **Server: GET /run** | ✅ | Returns all 5 monsters, all heroes, selected hero, XP rewards. Reads from `config.json`. |
| **Server: GET /move** | ✅ | Receives battle state, returns monster's chosen move index using weighted AI heuristics. |
| **Stats system** | ✅ | Health, Attack, Defense, Magic on all characters. Physical scales off ATK reduced by DEF. Magic scales off MAG, bypasses DEF. |
| **Moves system** | ✅ | 7 effect types: damage, heal, damage_heal, damage_debuff, buff, debuff, buff_self_damage. All 6 characters fully implemented with 4 moves each (24 unique moves). |

### Bonus Features

| # | Feature | Status | Details |
|---|---------|--------|---------|
| 1 | Move descriptions | ✅ | Full info panels on hover/select showing type, effect, base value, buff/debuff details. Shown in battle, char select, and move management. |
| 2 | Attribute choices on level up | ❌ | Fixed stat growth per class. |
| 3 | Status effects | ✅ | Buff/debuff system with stat modifiers and turn durations. Displayed in battle UI. |
| 4 | Resource costs | Partial | Dark Pact costs HP (buff_self_damage effect). No mana system. |
| 5 | Save & Exit | ❌ | — |
| 6 | Battle log | ✅ | Last hero and monster moves with results shown each turn. |
| 7 | Battle animations | ✅ | Full animation queue: damage numbers, hit flashes, sprite shakes, text popups, timed waits. |
| 8 | Smarter bot | ✅ | Weighted random AI: prefers heals when low HP, buffs when healthy, heavy damage on weak hero, avoids self-damage when low. |
| 9 | Items | ❌ | — |
| 10 | Shop | ❌ | — |
| 11 | More enemies & moves | ✅ | 5 monsters + Goblin Mage added. All configurable via web editor. |
| 12 | Non-linear map | ❌ | Linear gauntlet, but monster count is dynamic  |
| 13 | Environmental effects | ✅ | 5 unique battle backdrops: Forest Clearing, Dark Cave, Arcane Ruins, Haunted Swamp, Volcanic Lair. Procedurally generated ASCII art. |
| 14 | Endless mode | ❌ | — |
| 15 | Hero classes | ✅ | 4 playable classes (Knight, Mage, Priest, Dwarf) with unique sprites, base stats, growth rates, and starting movesets. Character selection screen. |

---

## Architecture

```
Client (C++20, Windows console)          Server (Python/Flask)
┌─────────────────────────┐              ┌──────────────────────┐
│  main_rpg.cpp           │  HTTP GET    │  app.py              │
│  └─ GameManager         │◄────────────►│  ├─ GET /run         │
│     ├─ MenuScreen       │  (WinHTTP)   │  ├─ GET /move        │
│     ├─ CharSelectScreen │              │  ├─ GET /api/config  │
│     ├─ MapScreen        │              │  ├─ POST /api/config │
│     ├─ BattleScreen     │              │  └─ GET / (editor)   │
│     ├─ PostBattleScreen │              └──────────┬───────────┘
│     ├─ MoveMgmtScreen   │                         │
│     └─ WinScreen        │              ┌──────────┴───────────┐
│                         │              │  config.json         │
│  Engine (ANSIITY)       │              │  editor.html         │
│  ├─ ANSIIRenderer       │              └──────────────────────┘
│  ├─ RenderManager       │
│  ├─ InputManager        │
│  └─ 240×60 console out  │
└─────────────────────────┘
```

### Key Files

| File | Purpose |
|------|---------|
| `main_rpg.cpp` | Entry point |
| `game/rpg/game_manager.h` | Screen orchestrator, run lifecycle |
| `game/rpg/rpg_state.h` | Data model, combat resolution, parsing |
| `game/rpg/battle_screen.h` | Battle UI, animation queue, turn execution |
| `game/rpg/backdrop.h` | Procedural backdrops for all screens |
| `game/rpg/screen.h` | Base Screen class + shared UI entities |
| `game/rpg/http_client.h` | WinHTTP synchronous client with timeouts |
| `game/rpg/json_parse.h` | Minimal JSON parser |
| `server/app.py` | Flask server with bot AI |
| `server/config.json` | All monsters, heroes, XP rewards |
| `server/editor.html` | Web-based config editor |

---

## Building from Source

### Prerequisites
- MinGW-w64 with g++ (C++20 support)
- Python 3.10+ with Flask (`pip install flask`)
- PyInstaller (`pip install pyinstaller`) for packaging

### Compile the game
```
cd ANSIITY-terminal-game-engine
g++ -O2 -o build/rpg.exe main_rpg.cpp -std=c++20 -I. -lwinhttp
```

### Run in development
```
# Terminal 1: start server
cd server
python app.py

# Terminal 2: start game
cd build
..\build\rpg.exe
```

### Package for distribution
```
# Bundle server into standalone exe
pyinstaller --onefile --name rpg_server server/app.py

# Assemble distributable
package.bat
```

Output: `dist/ANSIITY-RPG/` — zip and ship. Zero dependencies for end users.

---

## Config Editor

Run the server and open `http://127.0.0.1:5000/` to access the web editor. You can:

- Add/remove/edit monsters and heroes
- Tweak stats, moves, buff/debuff values
- Modify XP rewards per encounter
- Changes save to `config.json` and take effect on the next game run (no rebuild needed)
