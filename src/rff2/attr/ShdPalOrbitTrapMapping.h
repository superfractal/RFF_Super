//
// Created by Opus 5.5 on 2026-10-24
//

#pragma once

namespace merutilm::rff2 {
    // What the Shape trap's palette position follows; Distance is how every earlier version drew it.
    enum class ShdPalOrbitTrapMapping {
        // The palette runs along the log2 distance to the trap.
        DISTANCE = 0,
        // The palette turns with arg(z - c) at the nearest approach, Orbit Trap Arms times per turn.
        ANGLE = 1,
        // Contour bands of the log2 distance, a quarter cycle per halving times Density, with a dark seam between bands.
        BANDS = 2
    };
}
