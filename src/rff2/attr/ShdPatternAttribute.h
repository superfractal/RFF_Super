//
// Created by Opus 5 on 2026-08-07
// Modified by Opus 5 on 2026-08-17
//

#pragma once
#include <cstdint>
#include <glm/glm.hpp>

#include "ShdPatternInkMode.h"
#include "ShdPatternType.h"
#include "ShdTextureBlendMode.h"
#include "ShdTextureUVMode.h"

namespace merutilm::rff2 {
    // Stacked generated patterns, blended in index order over the escaping region. One layer draws
    // one shape at one Edge Width, and those two settle a single scale between them: a width that
    // leaves a thin seam draws the shape as a tile, and a width that swallows it leaves only its
    // peaks as grain. Both at once therefore needs two layers, which is what the stack is for.
    // Fixed rather than growable: every layer's parameters ride the texture set's own UBO.
    constexpr uint32_t PATTERN_LAYER_COUNT = 4;

    // Generated pattern painted over the escaping (non-black) region only; the interior keeps
    // mandelbrotColor. It needs no image file and stacks on top of the texture layer, sharing that
    // layer's UV sources so it can ride the color bands the same way.
    struct ShdPatternAttribute {
        bool enabled = false;
        ShdPatternType type = ShdPatternType::STRIPES;
        ShdTextureUVMode uvMode = ShdTextureUVMode::CYCLE_BAND;
        // Replace by default: the palette-shifted ink is already a color of its own, and multiplying
        // two palette colors together only darkens them.
        ShdTextureBlendMode blendMode = ShdTextureBlendMode::REPLACE;
        float opacity = 1.0f;
        ShdPatternInkMode inkMode = ShdPatternInkMode::PALETTE_SHIFT;
        // Ink drawn where the pattern covers, in SOLID mode. The uncovered part is left untouched.
        glm::vec4 color = {0.0f, 0.0f, 0.0f, 1.0f};
        // PALETTE_SHIFT mode: how far around the palette cycle the covered area is read, in cycles.
        // 0.5 lands on the opposite side of the cycle, the strongest contrast the palette offers.
        float paletteShift = 0.5f;
        // Edge hardness: 0 leaves the shape as a soft gradient, 1 cuts it to a hard border.
        float sharpness = 0.5f;
        // Pattern repeats across one full palette cycle (u) and across the v source range.
        float scaleU = 4.0f;
        float scaleV = 4.0f;
        // Pattern-space scroll in repeats per second; drives the pattern's own animation.
        float scrollU = 0.0f;
        float scrollV = 0.0f;

        // Share of the palette's own animation that U inherits. 1 rides the colors exactly,
        // 0 holds the pattern still while they animate, negative runs it against them.
        float paletteFollow = 1.0f;
        // Iterations spanned by one pattern tile along U. 0 follows the palette's cycle length,
        // which ties the pattern's size to the coloring; set it to size the pattern on its own.
        float periodIterations = 0.0f;
        // Outline straddling the border between the covered and uncovered parts of the shape, so it
        // reads as a seam between the two rather than a rim belonging to either one. Off by default:
        // every look saved before it existed was composed without one.
        bool edgeEnabled = false;
        // Outline ink, painted flat rather than through the blend mode - it is a separator, not a
        // second coat of the pattern. White by default, the strongest one over a saturated palette.
        glm::vec4 edgeColor = {1.0f, 1.0f, 1.0f, 1.0f};
        // Half-thickness of the seam, measured in the shape field the border is cut from. Small
        // enough to read as a line rather than a band of its own. Negative goes below the one ramp
        // the band is still wide at 0, down to where the line disappears at -0.5 on a soft shape.
        float edgeWidth = 0.15f;
        float edgeOpacity = 1.0f;
        // Measures the width against the shape's own peak instead of in raw field units. Every shape
        // has a different peak - Honeycomb reaches 0.38 where Diamond's outer side reaches 1.30 - so
        // in raw units the width that leaves only the peaks standing is a different number for each,
        // and each side of the same shape. Scaled, that point is 1 minus the shape's soft border for
        // all of them, which is what makes the grain the outline breaks into reachable on purpose.
        bool edgeRelative = false;
    };
}
