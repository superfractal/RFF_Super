//
// Created by Opus 5 on 2026-08-18.
// Modified by GPT-5 on 2026-08-18
// Modified by GPT-6 on 2026-09-23
//

#pragma once

namespace merutilm::rff2 {
    // How one timeline key reaches the next. Enums, booleans and file paths can only ever step.
    enum class VidKeyInterpolation {
        // Timeline files store these values as integers.
        STEP = 0,
        LINEAR = 1,
        SMOOTH = 2,
        CUBIC = 3
    };
}
