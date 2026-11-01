//
// Created by Opus 5.5 on 2026-10-12
//

#pragma once

namespace merutilm::rff2 {
    // How the orbit trap color is laid over the iteration palette, scaled by Orbit Trap Amount.
    enum class ShdPalOrbitTrapBlend {
        MIX = 0,
        MULTIPLY = 1,
        // 1 - (1 - base)(1 - trap): brightens only, so the palette keeps its colors under the glow.
        SCREEN = 2,
        ADD = 3
    };
}
