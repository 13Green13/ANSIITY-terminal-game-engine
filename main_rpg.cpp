#include "engine/engine.h"
#include "game/rpg/game_manager.h"

int main() {
    auto* manager = new GameManager();
    manager->init();

    // Build the initial menu screen
    manager->showMenu();

    Engine::getInstance().start();
    return 0;
}
