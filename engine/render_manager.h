#pragma once

#include <vector>
#include <algorithm>
#include <cstring>
#include <windows.h>
#include "ansii_renderer.h"
#include "camera.h"
#include "constants.h"
#include "profiler.h"

class RenderManager {
    std::vector<ANSIIRenderer*> _renderers;
    char _screenChars[WIDTH][HEIGHT] = {};
    int _screenColors[WIDTH][HEIGHT] = {};
    bool _screenOccupied[WIDTH][HEIGHT] = {};

    // WriteConsoleOutput buffer
    CHAR_INFO _consoleBuf[WIDTH * HEIGHT] = {};
    HANDLE _hOut = INVALID_HANDLE_VALUE;

    // Active camera (null = no offset)
    Camera* _activeCamera = nullptr;

    // Map ANSI color codes to Windows console attributes
    WORD _colorMap[256] = {};
    bool _colorMapInit = false;

    RenderManager() = default;

    void initColorMap() {
        if (_colorMapInit) return;
        // Default: white on black
        for (int i = 0; i < 256; i++) {
            _colorMap[i] = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        }
        // ANSI foreground colors → Windows console attributes
        _colorMap[30] = 0;                                                          // black
        _colorMap[31] = FOREGROUND_RED;                                             // red
        _colorMap[32] = FOREGROUND_GREEN;                                           // green
        _colorMap[33] = FOREGROUND_RED | FOREGROUND_GREEN;                          // yellow
        _colorMap[34] = FOREGROUND_BLUE;                                            // blue
        _colorMap[35] = FOREGROUND_RED | FOREGROUND_BLUE;                           // magenta
        _colorMap[36] = FOREGROUND_GREEN | FOREGROUND_BLUE;                         // cyan
        _colorMap[37] = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;        // white
        // Bright variants
        _colorMap[90] = FOREGROUND_INTENSITY;                                       // bright black
        _colorMap[91] = FOREGROUND_RED | FOREGROUND_INTENSITY;                      // bright red
        _colorMap[92] = FOREGROUND_GREEN | FOREGROUND_INTENSITY;                    // bright green
        _colorMap[93] = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;   // bright yellow
        _colorMap[94] = FOREGROUND_BLUE | FOREGROUND_INTENSITY;                     // bright blue
        _colorMap[95] = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;    // bright magenta
        _colorMap[96] = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;  // bright cyan
        _colorMap[97] = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; // bright white
        _colorMapInit = true;
    }

public:
    static RenderManager& getInstance() {
        static RenderManager instance;
        return instance;
    }

    RenderManager(const RenderManager&) = delete;
    void operator=(const RenderManager&) = delete;

    void subscribe(ANSIIRenderer* renderer) {
        // Insert in sorted order by zOrder (low first)
        auto it = std::lower_bound(_renderers.begin(), _renderers.end(), renderer,
            [](const ANSIIRenderer* a, const ANSIIRenderer* b) {
                return a->zOrder < b->zOrder;
            });
        _renderers.insert(it, renderer);
    }

    void setActiveCamera(Camera* cam) {
        _activeCamera = cam;
    }

    Camera* getActiveCamera() const {
        return _activeCamera;
    }

    void unsubscribe(ANSIIRenderer* renderer) {
        _renderers.erase(
            std::remove(_renderers.begin(), _renderers.end(), renderer),
            _renderers.end()
        );
    }

    void draw() {
        initColorMap();

        if (_hOut == INVALID_HANDLE_VALUE) {
            _hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        }

        Profiler& prof = Profiler::getInstance();

        // --- BUILD phase ---
        prof.beginSection();

        // Clear screen buffers
        std::memset(_screenChars, 0, sizeof(_screenChars));
        std::memset(_screenColors, 0, sizeof(_screenColors));
        std::memset(_screenOccupied, 0, sizeof(_screenOccupied));

        int camX = _activeCamera ? (int)_activeCamera->x : 0;
        int camY = _activeCamera ? (int)_activeCamera->y : 0;

        // Draw renderers high-z first (reverse) with occlusion culling
        for (int i = (int)_renderers.size() - 1; i >= 0; i--) {
            ANSIIRenderer* r = _renderers[i];
            // Frustum cull world-space renderers
            if (!r->screenSpace && _activeCamera) {
                if (!_activeCamera->isVisible(r->x, r->y, r->width, r->height)) {
                    continue;
                }
            }
            r->draw(_screenChars, _screenColors, _screenOccupied, camX, camY);
        }

        // Build CHAR_INFO buffer (row-major for WriteConsoleOutput)
        for (int j = 0; j < HEIGHT; j++) {
            for (int i = 0; i < WIDTH; i++) {
                int idx = j * WIDTH + i;
                _consoleBuf[idx].Char.AsciiChar = _screenChars[i][j];
                int color = _screenColors[i][j];
                _consoleBuf[idx].Attributes = (color > 0) ? _colorMap[color]
                    : (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            }
        }

        prof.recordRenderBuild(prof.endSection());

        // --- FLUSH phase ---
        prof.beginSection();

        // Single syscall to write entire screen
        COORD bufSize = { WIDTH, HEIGHT };
        COORD bufCoord = { 0, 0 };
        SMALL_RECT writeRegion = { 0, 0, (SHORT)(WIDTH - 1), (SHORT)(HEIGHT - 1) };
        WriteConsoleOutputA(_hOut, _consoleBuf, bufSize, bufCoord, &writeRegion);

        prof.recordRenderFlush(prof.endSection());
    }
};
