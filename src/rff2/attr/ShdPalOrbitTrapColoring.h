//
// Created by Opus 5.5 on 2026-10-12
// Modified by Opus 5.5 on 2026-10-16, 2026-11-01
//

#pragma once

namespace merutilm::rff2 {
    // Colors each pixel by how close its orbit came to a trap shape, a value the iteration count does not carry.
    enum class ShdPalOrbitTrapColoring {
        NONE = 0,
        // Clifford A. Pickover's epsilon cross; see NOTICE.
        // Nearest approach to the real and imaginary axes (Pickover stalks): the palette runs along the distance and glows near the axes.
        CROSS_GLOW = 1,
        // F1/F2 cells after Steven Worley, "A Cellular Texture Basis Function" (SIGGRAPH 1996); written independently, see NOTICE.
        // Nearest and second nearest approach to the origin: cells colored by the iteration of the nearest one, darkened toward their borders.
        CELLULAR = 2,
        // Nearest approach to the user's Orbit Trap Shape, glowing along the distance as Cross Glow does.
        SHAPE = 3
    };
}
