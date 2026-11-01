//
// Created by Merutilm on 8/27/26.
// Modified by Opus 5.5 on 2026-10-06
//

// Derived from Merutilm RFF 2.3.0 src/rff2/settings/RndComputeShader.hpp, GPL-3.0; see NOTICE.
#pragma once
#include <cstdint>

namespace merutilm::rff2 {
    // GPU Perturbation settings, named after their controls in the Render & Preview panel.
    struct RndComputeShaderAttribute {
        // Use GPU Perturbation, off by default; a device or view the GPU cannot take falls back to the CPU on its own.
        bool use = false;
        // Single Precision: float down to zoom 1e30, as RFF 2.3.0 did; much faster, but values drift from double.
        bool singlePrecision = true;
        // Batch Duration (s): each batch is lengthened or shortened toward this time.
        float batchSeconds = 0.1f;
        // Allowed Unfinished Pixels: the calculation ends once no more pixels than this are still iterating.
        uint32_t allowedUnfinishedPixels = 0;
        // Ignore MPA on GPU: iterate every step without approximation skips.
        bool ignoreMpa = false;
        // Stop MPA After Batches: later batches iterate without MPA; 0 keeps MPA throughout.
        uint32_t stopMpaAfterBatches = 0;
        // Interpolate Isolated Pixels: an unfinished pixel with four finished neighbors takes their mean.
        bool interpolateIsolatedPixels = false;

        bool operator==(const RndComputeShaderAttribute &) const = default;
    };
}
