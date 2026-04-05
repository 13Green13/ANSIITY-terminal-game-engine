#pragma once

// Override at compile time: -DSCREEN_WIDTH=120 -DSCREEN_HEIGHT=60
#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 240
#endif
#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 60
#endif

constexpr int WIDTH = SCREEN_WIDTH;
constexpr int HEIGHT = SCREEN_HEIGHT;
constexpr float TARGET_FPS = 60.0f;

// Sprite/tile transparency: cells with this color code are not drawn
constexpr int TRANSPARENT_COLOR = 0;
