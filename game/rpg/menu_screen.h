#pragma once

#include "screen.h"
#include "rpg_state.h"
#include "http_client.h"
#include "backdrop.h"
#include "../../engine/constants.h"

class MenuScreen : public Screen {
    RunState& _run;
    HttpClient& _http;
    int _selected = 0;
    std::function<void(int)> _onSelect; // 0 = start, 1 = exit
    bool _error = false;

public:
    MenuScreen(RunState& run, HttpClient& http, std::function<void(int)> onSelect)
        : _run(run), _http(http), _onSelect(std::move(onSelect)) {}

    void build() override {
        addEntity(makeMenuBackdrop());

        int cx = WIDTH / 2;
        int cy = HEIGHT / 3;

        std::string title = "=== ANSIITY RPG ===";
        addEntity(new TextEntity((float)(cx - (int)title.size() / 2), (float)cy, title, 96));

        std::string opt1 = "Start New Run";
        std::string opt2 = "Exit";
        addEntity(new TextEntity((float)(cx - (int)opt1.size() / 2), (float)(cy + 4), opt1, 97));
        addEntity(new TextEntity((float)(cx - (int)opt2.size() / 2), (float)(cy + 6), opt2, 97));

        // Cursor
        float cursorY = (float)(cy + 4 + _selected * 2);
        std::string curOpt = _selected == 0 ? opt1 : opt2;
        addEntity(new TextEntity((float)(cx - (int)curOpt.size() / 2 - 3), cursorY, ">", 93));

        if (_error) {
            addEntity(new TextEntity(cx - 28, (float)(cy + 10),
                "Failed to connect to server! Is it running on port 5000?", 91));
        }
    }

    void update(float dt) override {
        auto& input = InputManager::getInstance();

        bool needRebuild = false;
        if (input.isKeyPressed(VK_UP) && _selected > 0) { _selected--; needRebuild = true; }
        if (input.isKeyPressed(VK_DOWN) && _selected < 1) { _selected++; needRebuild = true; }

        if (input.isKeyPressed(VK_RETURN)) {
            if (_selected == 0) {
                std::string response = _http.get("/run");
                if (parseRunConfig(response, _run)) {
                    _error = false;
                    _onSelect(0);
                } else {
                    _error = true;
                    needRebuild = true;
                }
            } else {
                _onSelect(1);
            }
        }

        if (needRebuild) rebuild();
    }
};
