//
// Created by Opus 5 on 2026-08-05
//

#pragma once

namespace merutilm::rff2 {
    // How the sampled texel is combined with the palette color of the same pixel.
    enum class ShdTextureBlendMode {
        MULTIPLY = 0,
        OVERLAY = 1,
        REPLACE = 2
    };
}
