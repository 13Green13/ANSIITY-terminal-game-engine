#pragma once

#include "../../engine/engine.h"
#include "../../engine/constants.h"
#include <vector>

class TitleEntity : public Entity {
public:
    TitleEntity(float x, float y, const Props&) {
        static const char* font[7][5] = {
            // A
            {" ### ", "#   #", "#####", "#   #", "#   #"},
            // N
            {"#   #", "##  #", "# # #", "#  ##", "#   #"},
            // S
            {" ### ", "#    ", " ### ", "    #", " ### "},
            // I
            {"#####", "  #  ", "  #  ", "  #  ", "#####"},
            // I
            {"#####", "  #  ", "  #  ", "  #  ", "#####"},
            // T
            {"#####", "  #  ", "  #  ", "  #  ", "  #  "},
            // Y
            {"#   #", " # # ", "  #  ", "  #  ", "  #  "},
        };

        int letterW = 5, letterH = 5, gap = 2, numLetters = 7;
        int titleW = numLetters * letterW + (numLetters - 1) * gap;
        int titleStartX = (WIDTH - titleW) / 2;
        int titleStartY = HEIGHT / 2 - letterH / 2 - 2;

        const char* subtitle = "game engine";
        int subLen = 11;
        int subStartX = (WIDTH - subLen) / 2;
        int subStartY = titleStartY + letterH + 2;

        std::vector<SpriteCell> cells(WIDTH * HEIGHT, {' ', 0});

        for (int li = 0; li < numLetters; li++) {
            int baseX = titleStartX + li * (letterW + gap);
            for (int row = 0; row < letterH; row++) {
                for (int col = 0; col < letterW; col++) {
                    if (font[li][row][col] == '#') {
                        cells[(titleStartY + row) * WIDTH + baseX + col] = {'#', 96};
                    }
                }
            }
        }

        for (int i = 0; i < subLen; i++) {
            cells[subStartY * WIDTH + subStartX + i] = {subtitle[i], 37};
        }

        renderer = new ANSIIRenderer(0, 0);
        renderer->screenSpace = true;
        renderer->zOrder = 10;
        renderer->loadFromData(WIDTH, HEIGHT, cells);
    }
};
