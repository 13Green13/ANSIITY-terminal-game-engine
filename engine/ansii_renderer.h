#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <unordered_map>
#include "constants.h"

struct SpriteCell {
    char character;
    int colorCode;
};

struct CachedSprite {
    int width, height;
    std::vector<SpriteCell> cells;
};

class ANSIIRenderer {
public:
    float x = 0;
    float y = 0;
    int width = 0;
    int height = 0;
    int zOrder = 0;
    bool screenSpace = false;  // true = ignores camera, draws at absolute position

private:
    std::vector<SpriteCell> _cells;

public:
    ANSIIRenderer() = default;

    ANSIIRenderer(float startX, float startY) : x(startX), y(startY) {}

    virtual ~ANSIIRenderer() = default;

    bool loadFromFile(const std::string& filepath) {
        static std::unordered_map<std::string, CachedSprite> _cache;

        auto it = _cache.find(filepath);
        if (it != _cache.end()) {
            width = it->second.width;
            height = it->second.height;
            _cells = it->second.cells;
            return true;
        }

        std::ifstream file(filepath);
        if (!file.is_open()) return false;

        std::string line;
        // Line 1: widthxheight
        if (!std::getline(file, line)) return false;

        size_t xpos = line.find('x');
        if (xpos == std::string::npos) return false;

        width = std::stoi(line.substr(0, xpos));
        height = std::stoi(line.substr(xpos + 1));

        _cells.clear();
        _cells.reserve(width * height);

        // Remaining lines: space-separated cells, each is <colorcode><char>
        for (int row = 0; row < height; row++) {
            if (!std::getline(file, line)) return false;
            std::istringstream iss(line);
            std::string token;
            for (int col = 0; col < width; col++) {
                if (!(iss >> token)) return false;
                // Last char is the character, everything before is the color code
                SpriteCell cell;
                cell.character = token.back();
                cell.colorCode = std::stoi(token.substr(0, token.size() - 1));
                _cells.push_back(cell);
            }
        }

        _cache[filepath] = { width, height, _cells };
        return true;
    }

    void loadFromData(int w, int h, const std::vector<SpriteCell>& cells) {
        width = w;
        height = h;
        _cells = cells;
    }

    virtual void draw(char screenChars[WIDTH][HEIGHT], int screenColors[WIDTH][HEIGHT],
               bool screenOccupied[WIDTH][HEIGHT], int camX, int camY) const {
        int offsetX = screenSpace ? 0 : -camX;
        int offsetY = screenSpace ? 0 : -camY;
        int ix = (int)x + offsetX;
        int iy = (int)y + offsetY;
        int idx = 0;
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                int sx = ix + j;
                int sy = iy + i;
                if (sx >= 0 && sx < WIDTH && sy >= 0 && sy < HEIGHT) {
                    if (!screenOccupied[sx][sy] && _cells[idx].colorCode != TRANSPARENT_COLOR) {
                        screenChars[sx][sy] = _cells[idx].character;
                        screenColors[sx][sy] = _cells[idx].colorCode;
                        screenOccupied[sx][sy] = true;
                    }
                }
                idx++;
            }
        }
    }

    const std::vector<SpriteCell>& getCells() const { return _cells; }
};
