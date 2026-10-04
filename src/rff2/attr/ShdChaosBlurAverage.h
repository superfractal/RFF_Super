//
// Created by Opus 5.5 on 2026-10-03.
//

#pragma once

namespace merutilm::rff2 {
    // Where the Chaos Blur aperture averages its samples: in display gamma, or in linear light.
    enum class ShdChaosBlurAverage {
        GAMMA = 0,
        LINEAR = 1
    };
}
