//
// Created by Opus 5.5 on 2026-10-29
//

#pragma once

namespace merutilm::rff2 {
    // Which side of the set boundary the orbit trap colors; the numbers are what the merge and palette shaders read.
    enum class ShdPalOrbitTrapRegion {
        // Only the escaping pixels; Max Iterations pixels keep Mandelbrot Color.
        OUTSIDE = 0,
        // Every pixel with trap data, as every earlier version drew it.
        BOTH = 1,
        // Only the pixels that reach Max Iterations; the escaping pixels keep the iteration palette.
        INSIDE = 2
    };
}
