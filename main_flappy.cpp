#include "engine/engine.h"
#include "game/flappy/birb_entity.h"
#include "game/flappy/pype_spawner.h"
#include "game/flappy/counter_entity.h"
#include "game/flappy/sky_entity.h"

int main() {
    auto* sky = new SkyEntity();
    sky->init();

    auto* birb = new BirbEntity(30.0f, HEIGHT / 2.0f);
    birb->init();

    auto* spawner = new PypeSpawner();
    spawner->init();

    auto* counter = new CounterEntity();
    counter->init();

    Engine::getInstance().start();

    return 0;
}
