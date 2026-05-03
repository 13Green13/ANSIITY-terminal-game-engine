#pragma once

#include "screen.h"
#include "../../engine/constants.h"
#include <vector>

// ─── Battle backdrop generator ──────────────────────────────────────
//
// Creates a full-screen environmental backdrop for battles.
// Rendered at z=1 so all sprites/UI draw on top.

static constexpr int BACKDROP_H = 38;
static constexpr int GROUND_ROW = 24;  // ground surface — sprite feet land here

// Deterministic hash for procedural textures
inline int bdHash(int x, int y, int seed) {
    unsigned int h = (unsigned int)x * 374761393u + (unsigned int)y * 668265263u + (unsigned int)seed;
    h = (h ^ (h >> 13)) * 1274126177u;
    return (int)((h ^ (h >> 16)) & 0x7FFFFFFFu);
}

class BackdropEntity : public Entity {
public:
    BackdropEntity(int monsterIdx) {
        std::vector<SpriteCell> cells(WIDTH * BACKDROP_H, {' ', 30});

        switch (monsterIdx) {
            case 0: buildForest(cells);  break;
            case 1: buildCave(cells);    break;
            case 2: buildRuins(cells);   break;
            case 3: buildSwamp(cells);   break;
            case 4: buildVolcano(cells); break;
            default: buildForest(cells); break;
        }

        // Clear zones around hero/monster sprites so text & sprites stay readable
        // Hero zone: name+HP at y=5-7, sprite at (15, 8) up to ~30w x 16h
        clearRect(cells, 10, 4, 50, 25);
        // Monster zone: name+HP, sprite at (190, 8) up to ~35w x 16h
        clearRect(cells, 183, 4, 50, 25);

        renderer = new ANSIIRenderer(0, 0);
        renderer->zOrder = 1;
        renderer->screenSpace = true;
        renderer->loadFromData(WIDTH, BACKDROP_H, cells);
    }

private:
    void set(std::vector<SpriteCell>& c, int x, int y, char ch, int color) {
        if (x >= 0 && x < WIDTH && y >= 0 && y < BACKDROP_H)
            c[y * WIDTH + x] = {ch, color};
    }

    // Wipe a rectangular area to transparent so sprites/text draw cleanly
    void clearRect(std::vector<SpriteCell>& c, int rx, int ry, int rw, int rh) {
        for (int y = ry; y < ry + rh && y < BACKDROP_H; y++)
            for (int x = rx; x < rx + rw && x < WIDTH; x++)
                if (x >= 0 && y >= 0)
                    c[y * WIDTH + x] = {' ', 0}; // color 0 = transparent
    }

    // ─── 1. Forest Clearing (Goblin Warrior) ────────────────────────

    void buildForest(std::vector<SpriteCell>& c) {
        // Sky base — dark blue
        for (int y = 0; y < GROUND_ROW; y++)
            for (int x = 0; x < WIDTH; x++) {
                int r = bdHash(x, y, 10);
                if (r % 80 == 0) set(c, x, y, '.', 94);  // faint star
                else set(c, x, y, ' ', 34);
            }

        // Clouds (rows 1-3)
        for (int y = 1; y < 4; y++)
            for (int x = 0; x < WIDTH; x++) {
                int r = bdHash(x, y, 20);
                // Cluster clouds using a wave pattern
                float wave = sinf(x * 0.04f + y * 1.2f);
                if (wave > 0.5f && r % 4 != 0)
                    set(c, x, y, '.', 37);
            }

        // Tree canopy (rows 0-7) — dense on edges, gap in middle for sky
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < WIDTH; x++) {
                int r = bdHash(x, y, 100);
                // Dense on edges (x < 60 or x > 180), sparser in middle
                bool edge = (x < 55 || x > 185);
                bool midEdge = (x < 70 || x > 170);
                int density = edge ? 7 : (midEdge ? 4 : 2);
                // Thin out on lower canopy rows
                if (y >= 6) density = density * 2 / 3;

                if (r % 10 < density) {
                    char ch = "#@%&"[r % 4];
                    int color = (r % 3 == 0) ? 92 : 32;
                    set(c, x, y, ch, color);
                }
            }
        }

        // Canopy fringe (rows 8-9)
        for (int y = 8; y < 10; y++)
            for (int x = 0; x < WIDTH; x++) {
                int r = bdHash(x, y, 110);
                bool edge = (x < 50 || x > 190);
                if (edge && r % 10 < 3)
                    set(c, x, y, "&#"[r % 2], 32);
            }

        // Tree trunks — pairs of '|' on far edges
        int trunks[] = {6, 12, 22, 35, 48, 195, 208, 218, 225, 232};
        for (int tx : trunks) {
            for (int y = 5; y < GROUND_ROW; y++) {
                set(c, tx, y, '|', 33);
                if (tx + 1 < WIDTH) set(c, tx + 1, y, '|', 33);
            }
            // Roots at ground
            if (tx > 0) set(c, tx - 1, GROUND_ROW - 1, '/', 33);
            set(c, tx + 2, GROUND_ROW - 1, '\\', 33);
        }

        // Scattered grass in open area
        for (int y = 15; y < GROUND_ROW; y++)
            for (int x = 0; x < WIDTH; x++) {
                int r = bdHash(x, y, 200);
                if (r % 40 == 0) set(c, x, y, ',', 32);
                else if (r % 60 == 0) set(c, x, y, '\'', 92);
            }

        // Ground surface line — dense grass
        for (int x = 0; x < WIDTH; x++) {
            int r = bdHash(x, GROUND_ROW, 300);
            char ch = "^^,\".;w"[r % 7];
            set(c, x, GROUND_ROW, ch, (r % 3 == 0) ? 92 : 32);
        }

        // Below ground — earth
        for (int y = GROUND_ROW + 1; y < BACKDROP_H; y++)
            for (int x = 0; x < WIDTH; x++) {
                int r = bdHash(x, y, 400);
                if (r % 15 == 0)      set(c, x, y, 'o', 90);
                else if (r % 8 == 0)  set(c, x, y, ',', 33);
                else                  set(c, x, y, '.', 33);
            }
    }

    // ─── 2. Dark Cave (Giant Spider) ────────────────────────────────

    void buildCave(std::vector<SpriteCell>& c) {
        // Fill everything with dark stone
        for (int y = 0; y < BACKDROP_H; y++)
            for (int x = 0; x < WIDTH; x++)
                set(c, x, y, ' ', 30);

        // Stone ceiling (rows 0-4)
        for (int y = 0; y < 5; y++)
            for (int x = 0; x < WIDTH; x++) {
                int r = bdHash(x, y, 50);
                char ch = "#=#="[r % 4];
                set(c, x, y, ch, (r % 4 == 0) ? 37 : 90);
            }

        // Stalactites hanging from ceiling
        for (int sx = 3; sx < WIDTH; sx += 7) {
            int r = bdHash(sx, 0, 60);
            int len = 3 + r % 6;  // 3-8 cells long
            int startY = 4;
            for (int dy = 0; dy < len && (startY + dy) < GROUND_ROW; dy++) {
                char ch = (dy == len - 1) ? 'V' : '|';
                int color = (dy < 2) ? 90 : 37;
                set(c, sx, startY + dy, ch, color);
            }
        }

        // Cave walls on far sides
        for (int y = 4; y < BACKDROP_H; y++) {
            for (int x = 0; x < WIDTH; x++) {
                int r = bdHash(x, y, 70);
                // Left wall
                int wallThick = 8 + (int)(3.0f * sinf(y * 0.3f));
                if (x < wallThick) {
                    set(c, x, y, "#=."[r % 3], 90);
                }
                // Right wall
                int rWall = WIDTH - 8 + (int)(3.0f * sinf(y * 0.4f + 2.0f));
                if (x > rWall) {
                    set(c, x, y, "#=."[r % 3], 90);
                }
            }
        }

        // Cobwebs — upper corners
        // Top-left web
        for (int i = 0; i < 20; i++) {
            int wx = 8 + i * 2;
            int wy = 5 + i;
            if (wy < GROUND_ROW && wx < 60) {
                set(c, wx, wy, '\\', 37);
                if (i % 3 == 0) {
                    for (int j = 1; j < 4; j++)
                        if (wx + j < WIDTH) set(c, wx + j, wy, '-', 37);
                }
            }
        }
        // Top-right web
        for (int i = 0; i < 20; i++) {
            int wx = WIDTH - 10 - i * 2;
            int wy = 5 + i;
            if (wy < GROUND_ROW && wx > 180) {
                set(c, wx, wy, '/', 37);
                if (i % 3 == 0) {
                    for (int j = 1; j < 4; j++)
                        if (wx - j >= 0) set(c, wx - j, wy, '-', 37);
                }
            }
        }

        // Water drips in open area
        for (int y = 10; y < GROUND_ROW; y++)
            for (int x = 15; x < WIDTH - 15; x++) {
                int r = bdHash(x, y, 80);
                if (r % 120 == 0) set(c, x, y, '.', 36);
                else if (r % 200 == 0) set(c, x, y, '\'', 96);
            }

        // Stone floor
        for (int x = 0; x < WIDTH; x++) {
            int r = bdHash(x, GROUND_ROW, 90);
            char ch = "=#-=_"[r % 5];
            set(c, x, GROUND_ROW, ch, (r % 3 == 0) ? 37 : 90);
        }

        // Stalagmites on ground
        for (int sx = 10; sx < WIDTH - 10; sx += 12) {
            int r = bdHash(sx, GROUND_ROW, 95);
            int len = 2 + r % 3;
            for (int dy = 0; dy < len; dy++) {
                int gy = GROUND_ROW - 1 - dy;
                if (gy > 5) set(c, sx, gy, (dy == len - 1) ? '^' : '|', 90);
            }
        }

        // Below floor — dark stone with puddles
        for (int y = GROUND_ROW + 1; y < BACKDROP_H; y++)
            for (int x = 0; x < WIDTH; x++) {
                int r = bdHash(x, y, 100);
                if (r % 30 == 0)      set(c, x, y, '~', 34);
                else if (r % 6 == 0)  set(c, x, y, '.', 90);
                else                  set(c, x, y, '-', 90);
            }
    }

    // ─── 3. Arcane Ruins (Goblin Mage) ──────────────────────────────

    void buildRuins(std::vector<SpriteCell>& c) {
        // Night sky
        for (int y = 0; y < GROUND_ROW; y++)
            for (int x = 0; x < WIDTH; x++) {
                int r = bdHash(x, y, 30);
                if (r % 60 == 0)       set(c, x, y, '*', 93);
                else if (r % 90 == 0)  set(c, x, y, '.', 94);
                else                   set(c, x, y, ' ', 30);
            }

        // Broken pillars on far left
        int leftPillars[] = {8, 20, 40, 55};
        for (int px : leftPillars) {
            int r = bdHash(px, 0, 130);
            int topY = 5 + r % 8;  // varying broken heights
            // Pillar shaft
            for (int y = topY; y < GROUND_ROW; y++) {
                set(c, px, y, '#', 37);
                set(c, px + 1, y, '|', 37);
                set(c, px + 2, y, '#', 37);
            }
            // Broken top
            set(c, px - 1, topY, '/', 90);
            set(c, px, topY - 1, '_', 37);
            set(c, px + 1, topY - 1, '_', 37);
            set(c, px + 2, topY - 1, '_', 37);
            set(c, px + 3, topY, '\\', 90);
            // Rubble at base
            set(c, px - 1, GROUND_ROW, '#', 90);
            set(c, px + 3, GROUND_ROW, '#', 90);
        }

        // Broken pillars on far right
        int rightPillars[] = {185, 200, 215, 230};
        for (int px : rightPillars) {
            int r = bdHash(px, 0, 140);
            int topY = 5 + r % 8;
            for (int y = topY; y < GROUND_ROW; y++) {
                set(c, px, y, '#', 37);
                set(c, px + 1, y, '|', 37);
                set(c, px + 2, y, '#', 37);
            }
            set(c, px - 1, topY, '/', 90);
            set(c, px, topY - 1, '_', 37);
            set(c, px + 1, topY - 1, '_', 37);
            set(c, px + 2, topY - 1, '_', 37);
            set(c, px + 3, topY, '\\', 90);
            set(c, px - 1, GROUND_ROW, '#', 90);
            set(c, px + 3, GROUND_ROW, '#', 90);
        }

        // Floating magical particles in the arena
        for (int y = 5; y < GROUND_ROW; y++)
            for (int x = 60; x < 180; x++) {
                int r = bdHash(x, y, 150);
                if (r % 100 == 0)      set(c, x, y, '*', 95);
                else if (r % 150 == 0) set(c, x, y, '+', 35);
                else if (r % 200 == 0) set(c, x, y, '.', 94);
            }

        // Rune circle on the ground (centered around X=120)
        int cx = 120;
        int runeR = 25;
        for (int x = cx - runeR; x <= cx + runeR; x++) {
            float dx = (float)(x - cx);
            float dist = fabsf(dx);
            // Top and bottom of circle mapped to row space
            float circY = sqrtf((float)(runeR * runeR) - dx * dx) * 0.3f;
            int topRow = GROUND_ROW - (int)circY;
            int botRow = GROUND_ROW + (int)circY;
            int r = bdHash(x, topRow, 160);
            if (topRow > 8 && topRow < GROUND_ROW)
                set(c, x, topRow, "*+."[r % 3], (r % 2) ? 95 : 35);
            if (botRow < BACKDROP_H)
                set(c, x, botRow, "*+."[r % 3], (r % 2) ? 95 : 35);
        }

        // Cracked stone floor
        for (int x = 0; x < WIDTH; x++) {
            int r = bdHash(x, GROUND_ROW, 170);
            char ch = "=-_=.-"[r % 6];
            set(c, x, GROUND_ROW, ch, (r % 4 == 0) ? 37 : 90);
        }

        // Below ground — ancient stone
        for (int y = GROUND_ROW + 1; y < BACKDROP_H; y++)
            for (int x = 0; x < WIDTH; x++) {
                int r = bdHash(x, y, 180);
                if (r % 25 == 0)      set(c, x, y, '*', 35);  // buried rune
                else if (r % 5 == 0)  set(c, x, y, '.', 90);
                else                  set(c, x, y, '-', 90);
            }
    }

    // ─── 4. Haunted Swamp (Witch) ───────────────────────────────────

    void buildSwamp(std::vector<SpriteCell>& c) {
        // Dark sky
        for (int y = 0; y < GROUND_ROW; y++)
            for (int x = 0; x < WIDTH; x++) {
                int r = bdHash(x, y, 40);
                if (r % 100 == 0) set(c, x, y, '.', 37);  // dim star
                else              set(c, x, y, ' ', 30);
            }

        // Crescent moon (top-right area)
        int mx = 190, my = 3;
        const char* moon[] = {
            " ___ ",
            "/   \\",
            "|   |",
            "\\___/",
        };
        for (int dy = 0; dy < 4; dy++)
            for (int dx = 0; moon[dy][dx]; dx++)
                if (moon[dy][dx] != ' ')
                    set(c, mx + dx, my + dy, moon[dy][dx], 93);

        // Dead trees — twisted trunks and bare branches
        // Left-side dead trees
        int leftTrees[] = {10, 30, 50, 65};
        for (int tx : leftTrees) {
            int r = bdHash(tx, 0, 210);
            int height = 10 + r % 6;
            int topY = GROUND_ROW - height;
            // Trunk
            for (int y = topY + 3; y < GROUND_ROW; y++) {
                set(c, tx, y, '|', 33);
                // Slight lean
                if (y < topY + height / 2 && (r % 2))
                    set(c, tx - 1, y, '/', 33);
            }
            // Bare branches
            set(c, tx - 2, topY + 2, '/', 33);
            set(c, tx - 4, topY + 1, '/', 33);
            set(c, tx + 2, topY + 3, '\\', 33);
            set(c, tx + 3, topY + 2, '\\', 33);
            set(c, tx - 1, topY + 1, '/', 33);
            set(c, tx + 1, topY,     '\\', 33);
            set(c, tx - 3, topY,     '~', 33);
            set(c, tx + 4, topY + 1, '-', 33);
        }

        // Right-side dead trees
        int rightTrees[] = {175, 195, 210, 228};
        for (int tx : rightTrees) {
            int r = bdHash(tx, 0, 220);
            int height = 10 + r % 6;
            int topY = GROUND_ROW - height;
            for (int y = topY + 3; y < GROUND_ROW; y++)
                set(c, tx, y, '|', 33);
            set(c, tx + 2, topY + 2, '\\', 33);
            set(c, tx + 4, topY + 1, '\\', 33);
            set(c, tx - 2, topY + 3, '/', 33);
            set(c, tx - 3, topY + 2, '/', 33);
            set(c, tx + 1, topY + 1, '\\', 33);
            set(c, tx - 1, topY,     '/', 33);
            set(c, tx + 3, topY,     '~', 33);
            set(c, tx - 4, topY + 1, '-', 33);
        }

        // Fog wisps (rows 16-23)
        for (int y = 16; y < GROUND_ROW; y++)
            for (int x = 0; x < WIDTH; x++) {
                int r = bdHash(x, y, 230);
                float wave = sinf(x * 0.03f + y * 0.5f);
                if (wave > 0.3f && r % 6 < 2)
                    set(c, x, y, (r % 2) ? ':' : '.', 90);
            }

        // Ground — murky water
        for (int x = 0; x < WIDTH; x++) {
            int r = bdHash(x, GROUND_ROW, 240);
            char ch = "~~-~="[r % 5];
            set(c, x, GROUND_ROW, ch, (r % 3 == 0) ? 36 : 32);
        }

        // Below ground — dark water with lily pads
        for (int y = GROUND_ROW + 1; y < BACKDROP_H; y++)
            for (int x = 0; x < WIDTH; x++) {
                int r = bdHash(x, y, 250);
                if (r % 40 == 0)      set(c, x, y, 'o', 32);  // lily pad
                else if (r % 4 == 0)  set(c, x, y, '~', (r % 2) ? 36 : 34);
                else                  set(c, x, y, '~', 34);
            }

        // Cattails near ground line
        for (int sx = 8; sx < WIDTH - 8; sx += 15) {
            int r = bdHash(sx, GROUND_ROW, 260);
            if (r % 3 == 0) {
                for (int dy = 1; dy <= 4; dy++)
                    set(c, sx, GROUND_ROW - dy, '|', 32);
                set(c, sx, GROUND_ROW - 5, '*', 33);
            }
        }
    }

    // ─── 5. Volcanic Lair (Dragon) ──────────────────────────────────

    void buildVolcano(std::vector<SpriteCell>& c) {
        // Smoky dark sky
        for (int y = 0; y < GROUND_ROW; y++)
            for (int x = 0; x < WIDTH; x++) {
                int r = bdHash(x, y, 55);
                float smoke = sinf(x * 0.02f + y * 0.15f);
                if (smoke > 0.4f && y < 8)
                    set(c, x, y, (r % 2) ? ':' : '.', 90);
                else if (r % 50 == 0)
                    set(c, x, y, '*', (r % 2) ? 91 : 93);  // embers
                else
                    set(c, x, y, ' ', 30);
            }

        // Rocky formations on sides
        // Left rocks
        for (int y = 6; y < GROUND_ROW; y++) {
            int r = bdHash(y, 0, 270);
            int width = 10 + (int)(5.0f * sinf(y * 0.4f));
            for (int x = 0; x < width; x++) {
                int r2 = bdHash(x, y, 280);
                char ch = "#^#="[r2 % 4];
                set(c, x, y, ch, (r2 % 3 == 0) ? 37 : 90);
            }
        }
        // Right rocks
        for (int y = 6; y < GROUND_ROW; y++) {
            int r = bdHash(y, 1, 270);
            int width = 10 + (int)(5.0f * sinf(y * 0.5f + 1.0f));
            for (int x = WIDTH - width; x < WIDTH; x++) {
                int r2 = bdHash(x, y, 290);
                char ch = "#^#="[r2 % 4];
                set(c, x, y, ch, (r2 % 3 == 0) ? 37 : 90);
            }
        }

        // Lava cracks in the air (heat shimmer)
        for (int y = 12; y < GROUND_ROW; y++)
            for (int x = 20; x < WIDTH - 20; x++) {
                int r = bdHash(x, y, 300);
                if (r % 80 == 0) set(c, x, y, '~', 31);
            }

        // Ground — lava flow
        for (int x = 0; x < WIDTH; x++) {
            int r = bdHash(x, GROUND_ROW, 310);
            float wave = sinf(x * 0.06f);
            char ch = (wave > 0) ? '~' : '=';
            int color = (r % 3 == 0) ? 93 : 91;
            set(c, x, GROUND_ROW, ch, color);
        }

        // Lava glow on row above ground
        for (int x = 0; x < WIDTH; x++) {
            int r = bdHash(x, GROUND_ROW - 1, 315);
            if (r % 5 < 2) set(c, x, GROUND_ROW - 1, '.', 31);
        }

        // Below ground — intense lava
        for (int y = GROUND_ROW + 1; y < BACKDROP_H; y++)
            for (int x = 0; x < WIDTH; x++) {
                int r = bdHash(x, y, 320);
                float wave = sinf(x * 0.05f + y * 0.3f);
                char ch = (wave > 0) ? '~' : '=';
                int color;
                if (r % 5 == 0)      color = 93;  // bright yellow
                else if (r % 3 == 0) color = 91;  // bright red
                else                 color = 31;   // dark red
                set(c, x, y, ch, color);
            }

        // Rocky islands in the lava
        int islands[] = {60, 110, 150};
        for (int ix : islands) {
            int r = bdHash(ix, 30, 330);
            int w = 6 + r % 5;
            for (int y = GROUND_ROW + 2; y < GROUND_ROW + 5 && y < BACKDROP_H; y++)
                for (int dx = 0; dx < w; dx++)
                    set(c, ix + dx, y, '#', 90);
            // Top edge
            for (int dx = 0; dx < w; dx++)
                set(c, ix + dx, GROUND_ROW + 1, '^', 90);
        }
    }
};

// ═══════════════════════════════════════════════════════════════════════
// Screen backdrops — full-screen, very sparse to keep text readable
// ═══════════════════════════════════════════════════════════════════════

// Helper: fill entire screen-sized cell buffer
inline void bdSet(std::vector<SpriteCell>& c, int x, int y, char ch, int color) {
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
        c[y * WIDTH + x] = {ch, color};
}

inline Entity* makeFullBackdrop(const std::vector<SpriteCell>& cells) {
    Entity* e = new Entity();
    e->renderer = new ANSIIRenderer(0, 0);
    e->renderer->zOrder = 1;
    e->renderer->screenSpace = true;
    e->renderer->loadFromData(WIDTH, HEIGHT, cells);
    return e;
}

// ─── Menu Screen: Castle entrance with torches ──────────────────────

inline Entity* makeMenuBackdrop() {
    std::vector<SpriteCell> c(WIDTH * HEIGHT, {' ', 30});

    // Starry night sky (top half, very sparse)
    for (int y = 0; y < 30; y++)
        for (int x = 0; x < WIDTH; x++) {
            int r = bdHash(x, y, 500);
            if (r % 120 == 0)      bdSet(c, x, y, '.', 94);
            else if (r % 200 == 0) bdSet(c, x, y, '*', 37);
        }

    // Castle silhouette — two towers + wall across top
    // Left tower
    for (int y = 6; y < 35; y++) {
        for (int x = 20; x < 35; x++)
            bdSet(c, x, y, '#', 90);
    }
    // Left tower battlements
    for (int bx = 20; bx < 35; bx += 3) {
        bdSet(c, bx, 5, '#', 90);
        bdSet(c, bx + 1, 5, '#', 90);
    }

    // Right tower
    for (int y = 6; y < 35; y++) {
        for (int x = WIDTH - 35; x < WIDTH - 20; x++)
            bdSet(c, x, y, '#', 90);
    }
    for (int bx = WIDTH - 35; bx < WIDTH - 20; bx += 3) {
        bdSet(c, bx, 5, '#', 90);
        bdSet(c, bx + 1, 5, '#', 90);
    }

    // Connecting wall (behind title area — very dim)
    for (int y = 12; y < 35; y++)
        for (int x = 35; x < WIDTH - 35; x++) {
            int r = bdHash(x, y, 510);
            if (r % 12 == 0) bdSet(c, x, y, '.', 90);
        }

    // Gate archway (centered, below title)
    int gateL = WIDTH / 2 - 12;
    int gateR = WIDTH / 2 + 12;
    // Arch
    for (int x = gateL; x <= gateR; x++) {
        float dx = (float)(x - WIDTH / 2);
        int archY = 30 - (int)(sqrtf(fmaxf(0, 144.0f - dx * dx)) * 0.5f);
        for (int y = archY; y >= archY - 1 && y > 0; y--)
            bdSet(c, x, y, '#', 37);
    }
    // Gate pillars
    for (int y = 24; y < 50; y++) {
        bdSet(c, gateL, y, '#', 37);
        bdSet(c, gateL + 1, y, '|', 37);
        bdSet(c, gateR, y, '#', 37);
        bdSet(c, gateR - 1, y, '|', 37);
    }

    // Torches flanking gate
    int torchPositions[] = {gateL - 4, gateR + 4};
    for (int tx : torchPositions) {
        // Pole
        for (int y = 26; y < 40; y++)
            bdSet(c, tx, y, '|', 33);
        // Flame
        bdSet(c, tx, 25, '*', 93);
        bdSet(c, tx - 1, 24, ')', 91);
        bdSet(c, tx + 1, 24, '(', 91);
        bdSet(c, tx, 23, '.', 33);
        // Glow around flame
        bdSet(c, tx - 1, 25, '.', 33);
        bdSet(c, tx + 1, 25, '.', 33);
    }

    // Cobblestone ground (rows 45-59, very dim)
    for (int y = 45; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++) {
            int r = bdHash(x, y, 520);
            if (r % 6 == 0)      bdSet(c, x, y, '.', 90);
            else if (r % 10 == 0) bdSet(c, x, y, ',', 90);
        }

    // Decorative border (edges of screen)
    for (int x = 0; x < WIDTH; x++) {
        bdSet(c, x, 0, '=', 90);
        bdSet(c, x, HEIGHT - 1, '=', 90);
    }
    for (int y = 0; y < HEIGHT; y++) {
        bdSet(c, 0, y, '|', 90);
        bdSet(c, WIDTH - 1, y, '|', 90);
    }

    return makeFullBackdrop(c);
}

// ─── Character Select: Training arena ───────────────────────────────

inline Entity* makeCharSelectBackdrop() {
    std::vector<SpriteCell> c(WIDTH * HEIGHT, {' ', 30});

    // Wooden beams at top
    for (int x = 0; x < WIDTH; x++) {
        int r = bdHash(x, 0, 600);
        bdSet(c, x, 0, '=', 33);
        if (r % 20 == 0) bdSet(c, x, 1, '|', 33);
    }

    // Brick wall texture (very sparse to not conflict with text at x=90+ area)
    for (int y = 2; y < HEIGHT - 2; y++)
        for (int x = 0; x < WIDTH; x++) {
            int r = bdHash(x, y, 610);
            // Only place bricks along edges, leave center clear for content
            bool nearEdge = (x < 8 || x > WIDTH - 8);
            if (nearEdge && r % 4 < 2) {
                char ch = (y % 3 == 0) ? '-' : ((x % 8 == 0) ? '|' : ' ');
                if (ch != ' ') bdSet(c, x, y, ch, 90);
            }
        }

    // Weapon racks on far left wall
    int weapons[] = {8, 16, 24, 32, 40};
    for (int wy : weapons) {
        if (wy < HEIGHT - 5) {
            bdSet(c, 3, wy, '-', 37);
            bdSet(c, 4, wy, '-', 37);
            bdSet(c, 5, wy, '-', 37);
            // Weapon hanging
            int r = bdHash(4, wy, 620);
            char wpn = "/\\|+"[r % 4];
            bdSet(c, 4, wy + 1, wpn, 90);
        }
    }

    // Banners on far right wall
    int bannerXs[] = {WIDTH - 6, WIDTH - 4};
    for (int bx : bannerXs) {
        for (int y = 4; y < 20; y++) {
            int r = bdHash(bx, y, 630);
            bdSet(c, bx, y, '|', (r % 3 == 0) ? 31 : 91);
        }
        bdSet(c, bx, 20, 'V', 91);
        bdSet(c, bx, 3, '-', 37);
    }

    // Floor — wooden planks (very dim)
    for (int y = HEIGHT - 6; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++) {
            int r = bdHash(x, y, 640);
            if (x % 12 == 0)     bdSet(c, x, y, '|', 90);
            else if (r % 8 == 0) bdSet(c, x, y, '-', 90);
        }

    // Separator line between sprite area and stats
    for (int y = 5; y < HEIGHT - 8; y++) {
        int r = bdHash(80, y, 650);
        if (r % 3 != 0) bdSet(c, 80, y, ':', 90);
    }

    // Bottom border
    for (int x = 0; x < WIDTH; x++)
        bdSet(c, x, HEIGHT - 1, '=', 90);

    return makeFullBackdrop(c);
}

// ─── Map Screen: World map with terrain ─────────────────────────────

inline Entity* makeMapBackdrop() {
    std::vector<SpriteCell> c(WIDTH * HEIGHT, {' ', 30});

    // Parchment background — very sparse stipple in center
    for (int y = 5; y < HEIGHT - 3; y++)
        for (int x = 2; x < WIDTH - 2; x++) {
            int r = bdHash(x, y, 700);
            if (r % 60 == 0) bdSet(c, x, y, '.', 33);
        }

    // Parchment border — ornate frame
    for (int x = 1; x < WIDTH - 1; x++) {
        bdSet(c, x, 0, '=', 33);
        bdSet(c, x, HEIGHT - 1, '=', 33);
        bdSet(c, x, 5, '-', 90); // line under header
    }
    for (int y = 0; y < HEIGHT; y++) {
        bdSet(c, 0, y, '|', 33);
        bdSet(c, 1, y, ' ', 30);
        bdSet(c, WIDTH - 1, y, '|', 33);
        bdSet(c, WIDTH - 2, y, ' ', 30);
    }
    // Corners
    bdSet(c, 0, 0, '+', 33);
    bdSet(c, WIDTH - 1, 0, '+', 33);
    bdSet(c, 0, HEIGHT - 1, '+', 33);
    bdSet(c, WIDTH - 1, HEIGHT - 1, '+', 33);

    // Compass rose (bottom-right)
    int cx = WIDTH - 20;
    int cy = HEIGHT - 12;
    bdSet(c, cx, cy - 2, 'N', 37);
    bdSet(c, cx, cy - 1, '|', 90);
    bdSet(c, cx, cy, '+', 37);
    bdSet(c, cx, cy + 1, '|', 90);
    bdSet(c, cx, cy + 2, 'S', 37);
    bdSet(c, cx - 2, cy, '-', 90);
    bdSet(c, cx - 3, cy, 'W', 37);
    bdSet(c, cx + 2, cy, '-', 90);
    bdSet(c, cx + 3, cy, 'E', 37);

    // Path line connecting encounter slots (row 16, matching node y=15)
    for (int x = 10; x < 210; x++) {
        int r = bdHash(x, 16, 710);
        if (r % 4 != 0) bdSet(c, x, 16, '.', 90);
    }

    // Terrain doodles around encounter area (sparse, away from text)
    // Mountains (far background, rows 7-12)
    int mPeaks[] = {30, 75, 140, 180, 220};
    for (int px : mPeaks) {
        int r = bdHash(px, 7, 720);
        int h = 2 + r % 3;
        for (int dy = 0; dy < h; dy++) {
            int row = 12 - dy;
            int w = h - dy;
            for (int dx = -w; dx <= w; dx++)
                bdSet(c, px + dx, row, '^', 90);
        }
    }

    // Trees scattered (below encounter row, rows 22-30)
    for (int y = 22; y < 30; y++)
        for (int x = 5; x < WIDTH - 5; x++) {
            int r = bdHash(x, y, 730);
            if (r % 100 == 0) bdSet(c, x, y, '^', 32);
            else if (r % 120 == 0) bdSet(c, x, y, '|', 33);
        }

    // Water features (rows 32-38)
    for (int y = 32; y < 38; y++)
        for (int x = 60; x < 180; x++) {
            int r = bdHash(x, y, 740);
            float wave = sinf(x * 0.05f + y * 0.3f);
            if (wave > 0.5f && r % 6 < 2)
                bdSet(c, x, y, '~', 34);
        }

    return makeFullBackdrop(c);
}

// ─── Post-Battle: Radiant victory / somber defeat ───────────────────

inline Entity* makePostBattleBackdrop(bool heroWon) {
    std::vector<SpriteCell> c(WIDTH * HEIGHT, {' ', 30});

    if (heroWon) {
        // Victory — radiating lines from center
        int cx = WIDTH / 2;
        int cy = HEIGHT / 3;

        // Starburst rays (dim, won't clash with centered text)
        for (int angle = 0; angle < 16; angle++) {
            float a = angle * 3.14159f / 8.0f;
            float dx = cosf(a);
            float dy = sinf(a) * 0.5f; // squish vertically for console aspect
            for (int t = 8; t < 60; t++) {
                int px = cx + (int)(dx * t);
                int py = cy + (int)(dy * t);
                int r = bdHash(px, py, 800 + angle);
                if (r % 3 == 0 && t > 12) // sparse, skip inner ring for text
                    bdSet(c, px, py, '.', (t < 25) ? 93 : 33);
            }
        }

        // Scattered sparkles
        for (int y = 0; y < HEIGHT; y++)
            for (int x = 0; x < WIDTH; x++) {
                int r = bdHash(x, y, 810);
                // Avoid center text box (cx-20..cx+20, cy-2..cy+12)
                bool inTextZone = (x > cx - 25 && x < cx + 25 && y > cy - 3 && y < cy + 14);
                if (!inTextZone && r % 150 == 0)
                    bdSet(c, x, y, '*', (r % 3 == 0) ? 93 : 33);
            }

        // Laurel wreath hints (arcs around text)
        for (int i = 0; i < 30; i++) {
            float a = -0.8f + i * 0.055f;
            int lx = cx - 28 + (int)(25.0f * cosf(a));
            int ly = cy + 5 + (int)(12.0f * sinf(a));
            bdSet(c, lx, ly, '~', 32);
        }
        for (int i = 0; i < 30; i++) {
            float a = -0.8f + i * 0.055f;
            int lx = cx + 28 - (int)(25.0f * cosf(a));
            int ly = cy + 5 + (int)(12.0f * sinf(a));
            bdSet(c, lx, ly, '~', 32);
        }
    } else {
        // Defeat — dark with rain
        for (int y = 0; y < HEIGHT; y++)
            for (int x = 0; x < WIDTH; x++) {
                int r = bdHash(x, y, 820);
                // Rain streaks (diagonal, sparse)
                if ((x + y * 2) % 20 == 0 && r % 5 < 2)
                    bdSet(c, x, y, '/', 90);
                else if (r % 200 == 0)
                    bdSet(c, x, y, '.', 34);
            }

        // Dark clouds at top
        for (int y = 0; y < 6; y++)
            for (int x = 0; x < WIDTH; x++) {
                int r = bdHash(x, y, 830);
                float wave = sinf(x * 0.03f + y * 0.8f);
                if (wave > 0.1f && r % 3 != 0)
                    bdSet(c, x, y, '.', 90);
            }
    }

    // Bottom border
    for (int x = 0; x < WIDTH; x++)
        bdSet(c, x, HEIGHT - 1, '-', 90);

    return makeFullBackdrop(c);
}

// ─── Win Screen: Celebration ────────────────────────────────────────

inline Entity* makeWinBackdrop() {
    std::vector<SpriteCell> c(WIDTH * HEIGHT, {' ', 30});

    int cx = WIDTH / 2;
    int cy = HEIGHT / 3;

    // Firework bursts (placed away from center text)
    struct Firework { int x, y, seed; int color; };
    Firework fireworks[] = {
        {30,  10, 900, 91}, {60,  8,  910, 93}, {WIDTH - 60, 8,  920, 92},
        {WIDTH - 30, 10, 930, 95}, {50,  45, 940, 96}, {WIDTH - 50, 45, 950, 91},
        {20,  30, 960, 93}, {WIDTH - 20, 30, 970, 95},
    };
    for (auto& fw : fireworks) {
        // Radial burst
        for (int angle = 0; angle < 12; angle++) {
            float a = angle * 3.14159f / 6.0f;
            for (int t = 1; t < 6; t++) {
                int px = fw.x + (int)(cosf(a) * t * 2);
                int py = fw.y + (int)(sinf(a) * t);
                int r = bdHash(px, py, fw.seed);
                char ch = (t < 3) ? '*' : '.';
                bdSet(c, px, py, ch, (t < 3) ? fw.color : 90);
            }
        }
        // Center
        bdSet(c, fw.x, fw.y, '@', fw.color);
    }

    // Trophy (centered, above text)
    int ty = cy - 8;
    // Cup
    bdSet(c, cx - 3, ty, '\\', 93);
    bdSet(c, cx - 2, ty, '_', 93);
    bdSet(c, cx - 1, ty, '_', 93);
    bdSet(c, cx, ty, '_', 93);
    bdSet(c, cx + 1, ty, '_', 93);
    bdSet(c, cx + 2, ty, '_', 93);
    bdSet(c, cx + 3, ty, '/', 93);
    bdSet(c, cx - 2, ty + 1, '\\', 93);
    bdSet(c, cx + 2, ty + 1, '/', 93);
    bdSet(c, cx - 1, ty + 2, '\\', 93);
    bdSet(c, cx + 1, ty + 2, '/', 93);
    bdSet(c, cx, ty + 3, '|', 93);
    bdSet(c, cx - 2, ty + 4, '-', 93);
    bdSet(c, cx - 1, ty + 4, '-', 93);
    bdSet(c, cx, ty + 4, '-', 93);
    bdSet(c, cx + 1, ty + 4, '-', 93);
    bdSet(c, cx + 2, ty + 4, '-', 93);

    // Confetti scattered everywhere (avoid center text block)
    for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++) {
            int r = bdHash(x, y, 980);
            bool inText = (x > cx - 20 && x < cx + 20 && y > cy - 2 && y < cy + 12);
            if (!inText && r % 80 == 0) {
                char ch = "*.+~o"[r % 5];
                int color = (int[]){91, 92, 93, 94, 95, 96}[r % 6];
                bdSet(c, x, y, ch, color);
            }
        }

    return makeFullBackdrop(c);
}

// ─── Move Management: Library / study ───────────────────────────────

inline Entity* makeMoveMgmtBackdrop() {
    std::vector<SpriteCell> c(WIDTH * HEIGHT, {' ', 30});

    // Bookshelves on far right (beyond x=200, away from info panel at x=120)
    for (int y = 2; y < HEIGHT - 2; y++) {
        int r = bdHash(WIDTH - 5, y, 1000);
        // Shelf lines
        if (y % 6 == 0) {
            for (int x = WIDTH - 12; x < WIDTH - 1; x++)
                bdSet(c, x, y, '=', 33);
        } else {
            // Books
            for (int x = WIDTH - 11; x < WIDTH - 2; x++) {
                int r2 = bdHash(x, y, 1010);
                if (r2 % 3 < 2) {
                    char ch = (r2 % 2) ? '|' : ']';
                    int color = (int[]){31, 34, 32, 33, 35, 90}[r2 % 6];
                    bdSet(c, x, y, ch, color);
                }
            }
        }
    }

    // Candle on right edge
    bdSet(c, WIDTH - 7, 1, '*', 93);
    bdSet(c, WIDTH - 7, 2, '|', 37);

    // Vertical divider between equipped and learned sections
    for (int y = 10; y < HEIGHT - 4; y++) {
        int r = bdHash(50, y, 1020);
        if (r % 3 != 0) bdSet(c, 50, y, ':', 90);
    }

    // Vertical divider before info panel
    for (int y = 2; y < HEIGHT - 4; y++) {
        int r = bdHash(115, y, 1030);
        if (r % 3 != 0) bdSet(c, 115, y, ':', 90);
    }

    // Floor planks at bottom
    for (int y = HEIGHT - 3; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++) {
            int r = bdHash(x, y, 1040);
            if (x % 15 == 0)     bdSet(c, x, y, '|', 90);
            else if (r % 6 == 0) bdSet(c, x, y, '-', 90);
        }

    // Top shelf/ceiling
    for (int x = 0; x < WIDTH; x++)
        bdSet(c, x, 0, '=', 33);

    // Scroll decorations in corners
    bdSet(c, 2, 2, '/', 33);
    bdSet(c, 3, 3, '|', 33);
    bdSet(c, 2, HEIGHT - 3, '\\', 33);
    bdSet(c, 3, HEIGHT - 4, '|', 33);

    return makeFullBackdrop(c);
}
