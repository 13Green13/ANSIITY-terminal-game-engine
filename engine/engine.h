#pragma once

#include <windows.h>
#include <iostream>
#include "constants.h"
#include "entity.h"
#include "entity_manager.h"
#include "collision_manager.h"
#include "render_manager.h"
#include "input_manager.h"
#include "profiler.h"
#include "camera.h"

// ---- Deferred implementations that need all managers visible ----

inline void Entity::init() {
    EntityManager::getInstance().subscribe(this);
    if (renderer) {
        RenderManager::getInstance().subscribe(renderer);
    }
    if (collider) {
        CollisionManager::getInstance().subscribe(collider);
    }
}

inline void EntityManager::sweep() {
    for (int i = (int)_entities.size() - 1; i >= 0; i--) {
        Entity* e = _entities[i];
        if (!e->alive) {
            if (e->collider) {
                CollisionManager::getInstance().unsubscribe(e->collider);
            }
            if (e->renderer) {
                RenderManager::getInstance().unsubscribe(e->renderer);
            }
            delete e;
            _entities.erase(_entities.begin() + i);
        }
    }
}

// ---- Engine ----

class Engine {
    bool _running = false;

    Engine() = default;

public:
    static Engine& getInstance() {
        static Engine instance;
        return instance;
    }

    Engine(const Engine&) = delete;
    void operator=(const Engine&) = delete;

    void stop() {
        _running = false;
    }

    void start() {
        setupConsole();

        // Register engine's own key
        InputManager::getInstance().watchKey('Q');

        _running = true;

        LARGE_INTEGER freq, lastTime, currentTime;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&lastTime);

        Profiler& profiler = Profiler::getInstance();

        while (_running) {
            QueryPerformanceCounter(&currentTime);
            float dt = (float)(currentTime.QuadPart - lastTime.QuadPart) / freq.QuadPart;
            lastTime = currentTime;

            profiler.beginFrame();

            // Input
            profiler.beginSection();
            InputManager::getInstance().poll();
            if (InputManager::getInstance().isKeyPressed('Q')) {
                _running = false;
                break;
            }
            profiler.recordInput(profiler.endSection());

            // Update
            profiler.beginSection();
            EntityManager::getInstance().update(dt);
            // Physics step — runs after entity update so addForce() calls take effect
            for (auto* e : EntityManager::getInstance().getEntities()) {
                if (e->alive && e->physics) {
                    e->physics->update(dt);
                }
            }
            // Fire tile collision callbacks (after physics so Entity is fully visible)
            for (auto* e : EntityManager::getInstance().getEntities()) {
                if (e->alive && e->physics && !e->physics->getTileCollisions().empty()) {
                    for (auto& tc : e->physics->getTileCollisions()) {
                        e->onTileCollision(tc);
                    }
                }
            }
            profiler.recordUpdate(profiler.endSection());

            // Update active camera (follows target after entity positions update)
            {
                Camera* cam = RenderManager::getInstance().getActiveCamera();
                if (cam) cam->update(dt);
            }

            // Collision
            profiler.beginSection();
            CollisionManager::getInstance().resolve();
            profiler.recordCollision(profiler.endSection());

            // Render
            profiler.beginSection();
            RenderManager::getInstance().draw();
            profiler.recordRender(profiler.endSection());

            profiler.endFrame((int)EntityManager::getInstance().getEntities().size());

        }

        profiler.writeSummary();
        shutdownConsole();
    }

private:
    CONSOLE_FONT_INFOEX _originalFont = {};
    bool _fontSaved = false;
    int _zoomOutCount = 0;

    void setupConsole() {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

        // Save original font for restore
        _originalFont.cbSize = sizeof(CONSOLE_FONT_INFOEX);
        if (GetCurrentConsoleFontEx(hOut, FALSE, &_originalFont)) {
            _fontSaved = true;
        }

        // Shrink font until WIDTH x HEIGHT fits in the current window
        autoScaleFont(hOut);

        // Enable VT processing
        DWORD mode;
        GetConsoleMode(hOut, &mode);
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
        SetConsoleMode(hOut, mode);

        // Raw input
        HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
        GetConsoleMode(hIn, &mode);
        mode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
        SetConsoleMode(hIn, mode);

        // Set buffer to match our dimensions
        COORD bufSize = { (SHORT)WIDTH, (SHORT)HEIGHT };
        SetConsoleScreenBufferSize(hOut, bufSize);

        // Hide cursor
        printf("\033[?25l");
        printf("\033[2J\033[H");
        fflush(stdout);
    }

    void autoScaleFont(HANDLE hOut) {
        // Legacy Console API (works in conhost, ignored by Windows Terminal)
        {
            CONSOLE_FONT_INFOEX fontInfo = {};
            fontInfo.cbSize = sizeof(CONSOLE_FONT_INFOEX);
            fontInfo.FontFamily = FF_DONTCARE;
            fontInfo.FontWeight = FW_NORMAL;
            wcscpy(fontInfo.FaceName, L"Consolas");

            HWND consoleWnd = GetConsoleWindow();
            RECT clientRect;
            GetClientRect(consoleWnd, &clientRect);
            int pixelW = clientRect.right - clientRect.left;
            int pixelH = clientRect.bottom - clientRect.top;
            int fontH = pixelH / HEIGHT;
            int fontW = pixelW / WIDTH;
            int fontSize = fontH;
            if (fontW * 2 < fontSize) fontSize = fontW * 2;
            if (fontSize < 1) fontSize = 1;

            fontInfo.dwFontSize.X = 0;
            fontInfo.dwFontSize.Y = (SHORT)fontSize;
            SetCurrentConsoleFontEx(hOut, FALSE, &fontInfo);
        }

        // Windows Terminal: use Ctrl+/- to zoom until WIDTH x HEIGHT fits snugly
        if (GetEnvironmentVariableA("WT_SESSION", nullptr, 0) > 0) {
            HWND consoleWnd = GetConsoleWindow();
            SetForegroundWindow(consoleWnd);

            auto getSize = [&](int& cols, int& rows) {
                CONSOLE_SCREEN_BUFFER_INFO csbi;
                GetConsoleScreenBufferInfo(hOut, &csbi);
                cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
                rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
            };

            auto sendZoom = [](WORD key) {
                INPUT inputs[4] = {};
                inputs[0].type = INPUT_KEYBOARD;
                inputs[0].ki.wVk = VK_CONTROL;
                inputs[1].type = INPUT_KEYBOARD;
                inputs[1].ki.wVk = key;
                inputs[2].type = INPUT_KEYBOARD;
                inputs[2].ki.wVk = key;
                inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
                inputs[3].type = INPUT_KEYBOARD;
                inputs[3].ki.wVk = VK_CONTROL;
                inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
                SendInput(4, inputs, sizeof(INPUT));
                Sleep(50);
            };

            int cols, rows;
            getSize(cols, rows);

            // Phase 1: Zoom OUT if too few cells
            while ((cols < WIDTH || rows < HEIGHT) && _zoomOutCount < 80) {
                sendZoom(VK_OEM_MINUS);
                _zoomOutCount++;
                getSize(cols, rows);
            }

            // Phase 2: Zoom IN to find the tightest fit (largest font that still fits)
            // Try zooming in; if it breaks the fit, undo and stop
            int zoomInCount = 0;
            while (zoomInCount < 80) {
                sendZoom(VK_OEM_PLUS);
                int newCols, newRows;
                getSize(newCols, newRows);
                if (newCols < WIDTH || newRows < HEIGHT) {
                    // Went too far, undo last zoom in
                    sendZoom(VK_OEM_MINUS);
                    break;
                }
                zoomInCount++;
                _zoomOutCount--; // net effect on restore
            }
        }
    }

    void shutdownConsole() {
        // Restore zoom in Windows Terminal
        if (_zoomOutCount != 0) {
            HWND consoleWnd = GetConsoleWindow();
            SetForegroundWindow(consoleWnd);

            // Positive = we zoomed out net, restore with Ctrl+Plus
            // Negative = we zoomed in net, restore with Ctrl+Minus
            WORD restoreKey = (_zoomOutCount > 0) ? VK_OEM_PLUS : VK_OEM_MINUS;
            int steps = (_zoomOutCount > 0) ? _zoomOutCount : -_zoomOutCount;

            for (int i = 0; i < steps; i++) {
                INPUT inputs[4] = {};
                inputs[0].type = INPUT_KEYBOARD;
                inputs[0].ki.wVk = VK_CONTROL;
                inputs[1].type = INPUT_KEYBOARD;
                inputs[1].ki.wVk = restoreKey;
                inputs[2].type = INPUT_KEYBOARD;
                inputs[2].ki.wVk = restoreKey;
                inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
                inputs[3].type = INPUT_KEYBOARD;
                inputs[3].ki.wVk = VK_CONTROL;
                inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
                SendInput(4, inputs, sizeof(INPUT));
                Sleep(30);
            }
        }

        // Show cursor
        printf("\033[?25h");
        fflush(stdout);

        // Restore original font
        if (_fontSaved) {
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            SetCurrentConsoleFontEx(hOut, FALSE, &_originalFont);
        }

        // Restore input mode
        HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
        DWORD mode;
        GetConsoleMode(hIn, &mode);
        mode |= (ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
        SetConsoleMode(hIn, mode);
    }
};
