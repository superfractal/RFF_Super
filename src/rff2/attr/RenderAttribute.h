//
// Modified by AI; earlier exact modification date unavailable.
// Modified by Opus 5 on 2026-08-15, 2026-08-31
// Modified by GPT-5 on 2026-08-21
// Modified by GPT-6 on 2026-09-23
// Modified by Opus 5.5 on 2026-10-06, 2026-10-07, 2026-10-21
//

#pragma once
#include <cstdint>

#include "RndComputeShaderAttribute.h"
#include "RndSsaaFilter.h"

namespace merutilm::rff2 {
    struct RenderAttribute {
        float clarityMultiplier;
        uint32_t ssaa;
        float fps;
        bool linearInterpolation;
        uint32_t threads;
        bool boundaryTraceFill;
        bool preview2Color;
        bool coarsePreview;
        bool dither = true; // breaks the stair-step edge of a slow gradient before it is written as 8-bit
        RndComputeShaderAttribute computeShader = {}; // GPU perturbation for the double-precision zoom range
        RndSsaaFilter ssaaFilter = RndSsaaFilter::MITCHELL_LINEAR; // how Supersampling's samples become one pixel; old files load Box (Gamma)
    };
}
