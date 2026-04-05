#pragma once

#include <cmath>
#include "ansii_renderer.h"
#include "constants.h"

class Camera {
public:
    float x = 0;
    float y = 0;
    float smoothing = 0;       // 0 = instant snap, 0.9 = slow drift
    float deadZoneW = 0;
    float deadZoneH = 0;

private:
    ANSIIRenderer* _target = nullptr;

public:
    Camera() = default;
    Camera(float startX, float startY) : x(startX), y(startY) {}

    void follow(ANSIIRenderer* target, float smooth = 0) {
        _target = target;
        smoothing = smooth;
    }

    void unfollow() {
        _target = nullptr;
    }

    void update(float dt) {
        if (!_target) return;

        // Desired camera position: center target on screen
        float targetCamX = _target->x + _target->width / 2.0f - WIDTH / 2.0f;
        float targetCamY = _target->y + _target->height / 2.0f - HEIGHT / 2.0f;

        // Dead zone: don't move if target is within dead zone of current center
        if (deadZoneW > 0 || deadZoneH > 0) {
            float screenX = _target->x - x;
            float screenY = _target->y - y;
            float centerX = WIDTH / 2.0f;
            float centerY = HEIGHT / 2.0f;

            bool inDeadZone = (screenX >= centerX - deadZoneW / 2.0f &&
                               screenX + _target->width <= centerX + deadZoneW / 2.0f &&
                               screenY >= centerY - deadZoneH / 2.0f &&
                               screenY + _target->height <= centerY + deadZoneH / 2.0f);
            if (inDeadZone) return;
        }

        // Lerp toward target
        if (smoothing <= 0) {
            x = targetCamX;
            y = targetCamY;
        } else {
            float t = 1.0f - smoothing;
            x += (targetCamX - x) * t;
            y += (targetCamY - y) * t;
        }

        // Snap to pixel grid — prevents sub-pixel jitter where the
        // integer camera position alternates between 1px and 2px jumps
        x = std::round(x);
        y = std::round(y);
    }

    bool isVisible(float wx, float wy, int w, int h) const {
        return (wx + w > x && wx < x + WIDTH &&
                wy + h > y && wy < y + HEIGHT);
    }
};
