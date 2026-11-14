//
// Modified by GPT-6 on 2026-09-23
// Modified by Opus 5.5 on 2026-11-04, 2026-11-08
//

#pragma once

#include "ShdVignetteShape.h"

namespace merutilm::rff2 {
    // Presets provide these six values in declaration order.
    struct ShdColorAttribute {
        float gamma;
        float exposure;
        float hue;
        float saturation;
        float brightness;
        float contrast;
        // Vignette darkens the picture toward the frame's edges; 0 leaves the picture exactly as graded.
        float vignette = 0.0f;
        // Where the vignette starts, as a fraction of the way from the center (Ellipse) or the half height (Frame) to the edge.
        float vignetteRadius = 0.5f;
        // Saturation added just inside the darkened rim, so the edge deepens before it returns to the graded color.
        float vignetteRichness = 0.5f;
        ShdVignetteShape vignetteShape = ShdVignetteShape::ELLIPSE;
        // Red is drawn inward and blue outward by this many pixels at the corners of a 1080-line picture; 0 leaves the picture untouched.
        float chromaticAberration = 0.0f;
        // Where the fringe begins, from 0 at the center to 1 at the corners.
        float chromaticAberrationStart = 0.25f;
    };
}
