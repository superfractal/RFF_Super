//
// Created by Opus 5 on 2026-08-15.
//

#pragma once

namespace merutilm::rff2 {
    // Color space the blend between two adjacent palette entries runs in.
    enum class ShdPalColorInterpolationMethod {
        RGB = 0,
        OKLAB = 1
    };
}
