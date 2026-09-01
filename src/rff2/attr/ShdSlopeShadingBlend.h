//
// Created by Opus 5 on 2026-08-15.
// Modified by Opus 5 on 2026-08-16.
//

#pragma once

namespace merutilm::rff2 {
    // Composite the slope shading meets the palette color in. Values are the shader's mode numbers; 1 is left free for soft light.
    enum class ShdSlopeShadingBlend {
        OVERLAY = 0,
        OKLAB_LIGHTNESS = 2
    };
}
