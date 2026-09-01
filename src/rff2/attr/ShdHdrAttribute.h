//
// Created by Opus 5 on 2026-08-19.
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
    };
}
