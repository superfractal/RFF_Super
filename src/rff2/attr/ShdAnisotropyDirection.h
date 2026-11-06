//
// Created by Opus 5.5 on 2026-11-04
//

#pragma once

namespace merutilm::rff2 {
    // Which way Studio stretches an anisotropic highlight. Values are the shader's direction numbers and the stored file values.
    enum class ShdAnisotropyDirection {
        FIXED_ANGLE = 0,
        ALONG_BANDS = 1,
        ACROSS_BANDS = 2
    };
}
