#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdint>

struct TileDef {
    char character = ' ';
    int colorCode = 0;
    bool solid = false;
};

class TilePalette {
    TileDef _defs[256] = {};

public:
    TilePalette() = default;

    bool loadFromFile(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) return false;

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream iss(line);
            int id;
            char ch;
            int color;
            std::string solidStr;
            if (iss >> id >> ch >> color >> solidStr) {
                if (id >= 0 && id < 256) {
                    _defs[id].character = ch;
                    _defs[id].colorCode = color;
                    _defs[id].solid = (solidStr == "true");
                }
            }
        }
        return true;
    }

    const TileDef& get(uint8_t id) const { return _defs[id]; }
};

class TileMap {
    std::vector<uint8_t> _tiles;
    int _mapWidth = 0;
    int _mapHeight = 0;
    TilePalette* _palette = nullptr;

public:
    TileMap() = default;

    void setPalette(TilePalette* palette) { _palette = palette; }

    int getMapWidth() const { return _mapWidth; }
    int getMapHeight() const { return _mapHeight; }
    TilePalette* getPalette() const { return _palette; }
    const std::vector<uint8_t>& getTiles() const { return _tiles; }

    bool loadFromFile(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) return false;

        std::string line;
        // Line 1: widthxheight
        if (!std::getline(file, line)) return false;
        size_t xpos = line.find('x');
        if (xpos == std::string::npos) return false;

        _mapWidth = std::stoi(line.substr(0, xpos));
        _mapHeight = std::stoi(line.substr(xpos + 1));
        _tiles.resize((size_t)_mapWidth * _mapHeight, 0);

        // Remaining lines: rows of single-digit tile IDs (or space-separated for multi-digit)
        for (int row = 0; row < _mapHeight; row++) {
            if (!std::getline(file, line)) return false;
            for (int col = 0; col < _mapWidth && col < (int)line.size(); col++) {
                char ch = line[col];
                uint8_t tileId = 0;
                if (ch >= '0' && ch <= '9') tileId = ch - '0';
                else if (ch >= 'a' && ch <= 'z') tileId = 10 + (ch - 'a');
                else if (ch >= 'A' && ch <= 'Z') tileId = 36 + (ch - 'A');
                _tiles[(size_t)row * _mapWidth + col] = tileId;
            }
        }
        return true;
    }

    uint8_t getTile(int wx, int wy) const {
        if (wx < 0 || wx >= _mapWidth || wy < 0 || wy >= _mapHeight) return 0;
        return _tiles[(size_t)wy * _mapWidth + wx];
    }

    bool isSolid(int wx, int wy) const {
        if (!_palette) return false;
        return _palette->get(getTile(wx, wy)).solid;
    }

    void setTile(int wx, int wy, uint8_t tileId) {
        if (wx < 0 || wx >= _mapWidth || wy < 0 || wy >= _mapHeight) return;
        _tiles[(size_t)wy * _mapWidth + wx] = tileId;
    }
};
