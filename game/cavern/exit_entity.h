#pragma once

#include "../../engine/engine.h"

class ExitDoorEntity : public Entity {
public:
    ExitDoorEntity(float x, float y, const Props&) {
        tag = "exit";

        renderer = new ANSIIRenderer(x, y);
        renderer->loadFromFile("sprites/exit_door.ansii");
        renderer->zOrder = 15;

        collider = new ANSIICollider(renderer, this, 3);
        collider->layerMask[1] = false;
        collider->layerMask[2] = false;
        collider->layerMask[3] = false;
    }
};
