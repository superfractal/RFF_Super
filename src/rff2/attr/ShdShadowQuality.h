//
// Created by Opus 5.5 on 2026-11-14
//

#pragma once

namespace merutilm::rff2 {
    // How many taps the cast shadow marches toward the light. Values are the stored file values; the shader takes the tap count.
    enum class ShdShadowQuality {
        FAST = 0,
        STANDARD = 1,
        FINE = 2
    };

    // The tap count each quality marches.
    constexpr float shadowQualityTaps(const ShdShadowQuality quality) {
        switch (quality) {
            case ShdShadowQuality::FAST: return 8.0f;
            case ShdShadowQuality::FINE: return 32.0f;
            default: return 16.0f;
        }
    }
}
