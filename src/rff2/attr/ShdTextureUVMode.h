//
// Created by Opus 5 on 2026-08-05
//

#pragma once

namespace merutilm::rff2 {
    // Source of the (u, v) pair used to sample the exterior texture.
    enum class ShdTextureUVMode {
        CYCLE_ANGLE = 0,
        CYCLE_SCREEN = 1,
        SCREEN = 2,
        // v is the direction the iteration field climbs, so the texture turns with the bands
        // instead of smearing along them.
        CYCLE_BAND = 3
    };
}
