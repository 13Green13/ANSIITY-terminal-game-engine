#pragma once

#include "ansii_renderer.h"
#include "tilemap.h"
#include "constants.h"

class TileMapRenderer : public ANSIIRenderer {
    TileMap* _tileMap = nullptr;

public:
    TileMapRenderer(TileMap* tileMap) : _tileMap(tileMap) {
        // Set bounds to full map size (for frustum culling by RenderManager)
        x = 0;
        y = 0;
        width = tileMap->getMapWidth();
        height = tileMap->getMapHeight();
    }

    void draw(char screenChars[WIDTH][HEIGHT], int screenColors[WIDTH][HEIGHT],
              bool screenOccupied[WIDTH][HEIGHT], int camX, int camY) const override {
        if (!_tileMap || !_tileMap->getPalette()) return;

        // Only iterate the visible rectangle
        int startX = camX;
        int startY = camY;
        int endX = camX + WIDTH;
        int endY = camY + HEIGHT;

        // Clamp to map bounds
        if (startX < 0) startX = 0;
        if (startY < 0) startY = 0;
        if (endX > _tileMap->getMapWidth()) endX = _tileMap->getMapWidth();
        if (endY > _tileMap->getMapHeight()) endY = _tileMap->getMapHeight();

        const TilePalette* palette = _tileMap->getPalette();

        for (int wy = startY; wy < endY; wy++) {
            int sy = wy - camY;
            for (int wx = startX; wx < endX; wx++) {
                int sx = wx - camX;
                if (sx >= 0 && sx < WIDTH && sy >= 0 && sy < HEIGHT) {
                    if (!screenOccupied[sx][sy]) {
                        uint8_t tileId = _tileMap->getTile(wx, wy);
                        const TileDef& def = palette->get(tileId);
                        if (def.colorCode != TRANSPARENT_COLOR) {
                            screenChars[sx][sy] = def.character;
                            screenColors[sx][sy] = def.colorCode;
                            screenOccupied[sx][sy] = true;
                        }
                    }
                }
            }
        }
    }
};
