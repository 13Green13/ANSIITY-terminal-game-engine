#pragma once

#include <windows.h>
#include <cstring>
#include <vector>
#include <algorithm>

class InputManager {
    bool _currentState[256] = {};
    bool _previousState[256] = {};
    std::vector<int> _watchedKeys;

    InputManager() = default;

public:
    static InputManager& getInstance() {
        static InputManager instance;
        return instance;
    }

    InputManager(const InputManager&) = delete;
    void operator=(const InputManager&) = delete;

    void watchKey(int vk) {
        if (std::find(_watchedKeys.begin(), _watchedKeys.end(), vk) == _watchedKeys.end()) {
            _watchedKeys.push_back(vk);
        }
    }

    // Called once per frame by Engine, before entity updates
    void poll() {
        for (int vk : _watchedKeys) {
            _previousState[vk] = _currentState[vk];
            _currentState[vk] = (GetAsyncKeyState(vk) & 0x8000) != 0;
        }
    }

    // Key is currently down
    bool isKeyHeld(int vk) const {
        return _currentState[vk];
    }

    // Key was just pressed this frame (down now, wasn't last frame)
    bool isKeyPressed(int vk) const {
        return _currentState[vk] && !_previousState[vk];
    }

    // Key was just released this frame (up now, was down last frame)
    bool isKeyReleased(int vk) const {
        return !_currentState[vk] && _previousState[vk];
    }
};
