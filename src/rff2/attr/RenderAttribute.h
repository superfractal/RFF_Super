//
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21.
// Modified by Opus 5 on 2026-08-15, 2026-08-31
//

#pragma once

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
    };
}
