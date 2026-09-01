//
// Created by Opus 5 on 2026-08-18.
// Modified by GPT-5 on 2026-08-18
//

#pragma once

namespace merutilm::rff2 {
    // How one timeline key reaches the next. Enums, booleans and file paths can only ever step.
    enum class VidKeyInterpolation {
        STEP,
        LINEAR,
        SMOOTH,
        CUBIC
    };
}
