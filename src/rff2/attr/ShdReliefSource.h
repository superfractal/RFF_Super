//
// Created by Opus 5.5 on 2026-11-07
//

#pragma once

namespace merutilm::rff2 {
    // Where Slope takes the fine slope of its relief from. Values are the shader's numbers and the stored file values.
    enum class ShdReliefSource {
        ITERATIONS = 0,
        DZDC = 1
    };
}
