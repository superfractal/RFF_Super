//
// Created by Merutilm on 2025-05-04.
//
// Modified by Opus 5 on 2026-08-07, 2026-08-17, 2026-08-19
//

#pragma once
#include "ShdFogBlurQuality.h"


namespace merutilm::rff2 {
    struct ShdFogAttribute {
        float radius;
        float opacity;
        // Normalized distance from the frame centre where the fog starts; 0 = whole frame, 1 = edge only.
        float centerStart = 0.0f;
        // Flips the falloff so the fog covers the centre and clears outwards instead.
        bool centerInvert = false;
        // 0 = fog over the whole frame, 1 = fog only where the slope pass's rim light lands.
        float rimMask = 0.0f;
        // Saturates the rim footprint so the masked band reaches full fog strength; 1 = raw footprint.
        float rimMaskBoost = 20.0f;
        // Full-resolution blur radius the masked band dissolves into, in 1280-wide-relative pixels.
        float rimBlur = 6.0f;
        // Focus band: depth of field cut on the iteration count rather than on the screen, so one
        // range of the fractal's own depth stays sharp and the rest melts. Measured as a fraction of
        // the frame's maximum iteration, which keeps the chosen band in the same place as the zoom
        // deepens and the count grows. 0 amount leaves the pass exactly as it was.
        float focusAmount = 0.0f;
        float focusRatio = 0.5f;     // where the sharp band sits, 0 = shallowest, 1 = the interior edge
        float focusRange = 0.25f;    // half-width of the sharp band
        float focusFalloff = 1.0f;   // curve out of the band; >1 holds the sharp part wider
        float focusBlur = 10.0f;     // blur radius at full defocus, in 1280-wide-relative pixels
        // Speed holds the two full-resolution blurs above to a 16-texel ceiling, which shrinks against
        // the frame as the render resolution grows; Appearance spends the radius they ask for instead.
        // A fresh session starts on Appearance: below the ceiling the two modes render the same picture
        // for the same cost, so the extra fetches are paid only where Speed would have changed the
        // picture. Files written before the setting existed are put back on Speed as they are read.
        ShdFogBlurQuality blurQuality = ShdFogBlurQuality::APPEARANCE;
    };
}