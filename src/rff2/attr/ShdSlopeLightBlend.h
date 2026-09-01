//
// Created by Opus 5 on 2026-08-16.
//

#pragma once

namespace merutilm::rff2 {
    // Where the specular highlight and the rim light are added to the shaded color. Values are the
    // shader's mode numbers. DIRECT is the composite every version up to 2.0.8 used, and stays the
    // default so a settings file written by one of those looks as it did.
    enum class ShdSlopeLightBlend {
        DIRECT = 0,
        LINEAR = 1
    };
}
