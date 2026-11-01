//
// Created by Opus 5.5 on 2026-10-09.
//

#pragma once

namespace merutilm::rff2 {
    // The outline of the Chaos Blur aperture: a circle, or a regular polygon with this many blades.
    enum class ShdChaosBokehShape {
        CIRCLE = 0,
        TRIANGLE = 1,
        SQUARE = 2,
        PENTAGON = 3,
        HEXAGON = 4,
        HEPTAGON = 5,
        OCTAGON = 6
    };

    // The blade count the shader draws for a shape; 0 is the circle.
    inline float chaosBokehBlades(const ShdChaosBokehShape shape) {
        return shape == ShdChaosBokehShape::CIRCLE ? 0.0f : static_cast<float>(static_cast<int>(shape) + 2);
    }
}
