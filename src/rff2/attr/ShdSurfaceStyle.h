//
// Modified by GPT-6 on 2026-09-12, 2026-09-13, 2026-09-20, 2026-09-23
//

#pragma once

namespace merutilm::rff2 {
    enum class ShdSurfaceBlend {
        MIX = 0,
        ADD = 1,
        MULTIPLY = 2,
        SCREEN = 3
    };

    // Serialized IDs 7 and 8 are retired and must not be reused.
    enum class ShdSurfaceStyle {
        ORIGINAL = 0,
        LIQUID_METAL = 1,
        CYBER_SIGILISM = 2,
        PHONK = 3,
        BLACK_METAL = 4,
        DEEP_SEA = 5,
        UKIYO_E = 6
    };
}
