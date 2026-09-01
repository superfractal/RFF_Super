//
// Created by Opus 5 on 2026-08-07
//

#pragma once

namespace merutilm::rff2 {
    // Where the pattern's covered area takes its color from.
    enum class ShdPatternInkMode {
        // One flat color chosen in the picker.
        SOLID = 0,
        // The palette's own color, read further along the cycle. The pattern stays inside the
        // coloring you already chose, so it comes out as colored as the fractal instead of flat.
        PALETTE_SHIFT = 1
    };
}
