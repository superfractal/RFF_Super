//
// Created by Merutilm on 2025-05-04.
// Modified by GPT-6 on 2026-09-23
//

#pragma once

#include <cstdint>

namespace merutilm::rff2 {
    struct FrtReferenceCompAttribute final {
        uint32_t compressCriteria;
        uint8_t compressionThresholdPower;
        bool noCompressorNormalization;
    };
}
