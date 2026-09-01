//
// Created by Opus 5 on 2026-08-19.
//

#pragma once

namespace merutilm::rff2 {
    // The curve that pulls the float chain's above-white light back into the display's range.
    enum class ShdToneMapMethod {
        CLIP = 0,
        REINHARD = 1,
        ACES = 2,
        FILMIC = 3
    };
}
