//
// Created by Opus 5 on 2026-08-15
//

#pragma once

namespace merutilm::rff2 {
    // Field the domain warp reads its displacement from. NOISE needs no image file; the layer
    // choices read the luminance of that texture layer's own image, whether or not it is painting.
    enum class ShdWarpSource {
        NOISE = 0,
        TEXTURE_1 = 1,
        TEXTURE_2 = 2,
        TEXTURE_3 = 3,
        TEXTURE_4 = 4
    };
}
