//
// Created by Opus 5 on 2026-08-19.
// Modified by GPT-6 on 2026-09-16, 2026-09-23
//

#pragma once

#include "ShdToneMapMethod.h"

namespace merutilm::rff2 {
    struct ShdHdrAttribute {
        // Off clamps the chain back to 0..1 at the final pass, which is what every earlier build drew.
        bool use = false;
        // Stops applied in linear light before the curve, so the whole picture rides up or down the shoulder.
        float exposure = 0.0f;
        // The linear value the curve lands on display white; everything above it is what the shoulder rolls off.
        float headroom = 2.0f;
        ShdToneMapMethod method = ShdToneMapMethod::ACES;
        float mfrPeakNits = 4000.0f;

        bool isMfr() const {
            return use && method >= ShdToneMapMethod::MFR_SHOULDER &&
                   method <= ShdToneMapMethod::MFR_FALSE_COLOR;
        }
    };
}
