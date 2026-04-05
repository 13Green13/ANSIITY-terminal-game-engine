#pragma once

namespace CavernState {
    inline int gems = 0;
    inline int totalGems = 0;
    inline bool won = false;

    inline void reset() {
        gems = 0;
        totalGems = 0;
        won = false;
    }
}
