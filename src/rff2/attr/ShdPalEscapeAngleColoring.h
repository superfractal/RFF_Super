//
// Created by Opus 5.5 on 2026-10-07.
//

#pragma once

namespace merutilm::rff2 {
    // Shifts the palette by the angle of z where each orbit escaped, a value the iteration count does not carry.
    enum class ShdPalEscapeAngleColoring {
        NONE = 0,
        // The half of every band whose escaping z has a negative imaginary part moves by Amount cycles.
        BINARY_DECOMPOSITION = 1,
        // Every pixel moves by Amount cycles per full turn of the angle.
        TWIST = 2
    };
}
