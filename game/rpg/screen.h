#pragma once

#include "../../engine/engine.h"
#include "../../engine/input_manager.h"
#include <vector>
#include <string>

// ─── Shared UI primitives ───────────────────────────────────────────

inline ANSIIRenderer* makeTextRenderer(float x, float y, const std::string& text, int color, int zOrder = 50) {
    auto* r = new ANSIIRenderer(x, y);
    r->zOrder = zOrder;
    r->screenSpace = true;
    std::vector<SpriteCell> cells;
    cells.reserve(text.size());
    for (char c : text) {
        cells.push_back({c, color});
    }
    r->loadFromData((int)text.size(), 1, cells);
    return r;
}

class TextEntity : public Entity {
public:
    TextEntity(float x, float y, const std::string& text, int color, int zOrder = 50) {
        renderer = makeTextRenderer(x, y, text, color, zOrder);
    }
};

class SpriteEntity : public Entity {
public:
    SpriteEntity(float x, float y, const std::string& file, int zOrder = 10) {
        renderer = new ANSIIRenderer(x, y);
        renderer->zOrder = zOrder;
        renderer->screenSpace = true;
        renderer->loadFromFile(file);
    }
};

class HPBarEntity : public Entity {
    int _maxHP;
    int* _currentHP;
    int _barWidth;
    int _color;

public:
    HPBarEntity(float x, float y, int maxHP, int* currentHP, int barWidth, int color)
        : _maxHP(maxHP), _currentHP(currentHP), _barWidth(barWidth), _color(color) {
        renderer = new ANSIIRenderer(x, y);
        renderer->zOrder = 60;
        renderer->screenSpace = true;
        rebuild();
    }

    void rebuild() {
        int filled = (_barWidth * (*_currentHP)) / std::max(1, _maxHP);
        filled = std::max(0, std::min(filled, _barWidth));
        std::vector<SpriteCell> cells(_barWidth + 2);
        cells[0] = {'[', 37};
        for (int i = 0; i < _barWidth; i++) {
            cells[i + 1] = {i < filled ? '=' : ' ', i < filled ? _color : 90};
        }
        cells[_barWidth + 1] = {']', 37};
        renderer->loadFromData(_barWidth + 2, 1, cells);
    }

    void update(float dt) override { rebuild(); }
};

// ─── Animation system ───────────────────────────────────────────────

// A timed visual effect entity that self-destructs after a duration
class AnimEntity : public Entity {
    float _timer;
    float _duration;
    float _startX, _startY;
    float _velX, _velY;   // drift per second
    int _fadeColor;        // color to fade to at end

public:
    AnimEntity(float x, float y, const std::string& text, int color,
               float duration, float velX = 0, float velY = 0, int zOrder = 80)
        : _timer(0), _duration(duration), _startX(x), _startY(y),
          _velX(velX), _velY(velY), _fadeColor(90) {
        renderer = makeTextRenderer(x, y, text, color, zOrder);
    }

    void update(float dt) override {
        _timer += dt;
        if (_timer >= _duration) {
            destroy();
            return;
        }
        // Drift
        renderer->x = _startX + _velX * _timer;
        renderer->y = _startY + _velY * _timer;
    }
};

// Flash overlay — briefly tints a rectangular area with a color
class FlashEntity : public Entity {
    float _timer;
    float _duration;

public:
    FlashEntity(float x, float y, int w, int h, int color, float duration, int zOrder = 70)
        : _timer(0), _duration(duration) {
        renderer = new ANSIIRenderer(x, y);
        renderer->zOrder = zOrder;
        renderer->screenSpace = true;
        std::vector<SpriteCell> cells(w * h);
        for (auto& c : cells) { c.character = ' '; c.colorCode = color; }
        // Use block characters for visibility
        for (int row = 0; row < h; row++) {
            for (int col = 0; col < w; col++) {
                cells[row * w + col] = {'#', color};
            }
        }
        renderer->loadFromData(w, h, cells);
    }

    void update(float dt) override {
        _timer += dt;
        if (_timer >= _duration) {
            destroy();
        }
    }
};

// ─── Screen base class ──────────────────────────────────────────────

struct RunState;  // forward decl

class Screen {
protected:
    std::vector<Entity*> _entities;

public:
    virtual ~Screen() { cleanup(); }

    Entity* addEntity(Entity* e) {
        e->init();
        _entities.push_back(e);
        return e;
    }

    void cleanup() {
        for (auto* e : _entities) {
            e->destroy();
        }
        _entities.clear();
    }

    void rebuild() {
        cleanup();
        build();
    }

    virtual void build() = 0;
    virtual void update(float dt) = 0;
};
