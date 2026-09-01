//
// Created by Opus 5 on 2026-08-31.
//

#pragma once

namespace merutilm::rff2 {
    // The curve the iteration count is read through before it lands on the palette cycle. Every
    // curve is normalized to reach the end of its first cycle at exactly one Cycle Length, so the
    // setting keeps its meaning across all of them.
    enum class ShdPalIterationColoringMode {
        LINEAR = 0,
        SQUARE_ROOT = 1,
        CUBE_ROOT = 2,
        LOG = 3,
        LOG_LOG = 4,
        SMOOTHSTEP = 5,
        SMOOTHERSTEP = 6
    };
}
