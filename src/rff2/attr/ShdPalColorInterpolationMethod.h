//
// Created by Opus 5 on 2026-08-15.
// Modified by GPT-6 on 2026-09-10
//

#pragma once

namespace merutilm::rff2 {
    // Color space the blend between two adjacent palette entries runs in.
    enum class ShdPalColorInterpolationMethod {
        RGB = 0,
        OKLAB = 1,
        LINEAR_RGB = 2
    };
}
