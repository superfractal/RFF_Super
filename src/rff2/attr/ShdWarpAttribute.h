//
// Created by Opus 5 on 2026-08-15
//

#pragma once

#include "ShdTextureUVMode.h"
#include "ShdWarpSource.h"

namespace merutilm::rff2 {
    // Domain warp. Instead of painting over the color, this displaces the palette position each
    // pixel is read at, so the color bands themselves bend and swirl. It runs before the texture
    // and pattern layers, which therefore ride the bent bands rather than the straight ones.
    struct ShdWarpAttribute {
        bool enabled = false;
        ShdWarpSource source = ShdWarpSource::NOISE;
        ShdTextureUVMode uvMode = ShdTextureUVMode::CYCLE_BAND;
        // Palette cycles the lookup is pushed by at the field's extremes. 0 leaves the coloring alone.
        float amount = 0.25f;
        // Octaves of the generated field. 1 is a smooth swirl, higher adds turbulence within it.
        float octaves = 4.0f;
        // Warp field repeats across one full palette cycle (u) and across the v source range.
        float scaleU = 4.0f;
        float scaleV = 4.0f;
        // Warp-space scroll in repeats per second; drives the warp's own animation.
        float scrollU = 0.0f;
        float scrollV = 0.0f;
        // Share of the palette's own animation that U inherits. 1 rides the colors exactly,
        // 0 holds the warp still while they animate, negative runs it against them.
        float paletteFollow = 1.0f;
        // Iterations spanned by one warp tile along U. 0 follows the palette's cycle length.
        float periodIterations = 0.0f;
    };

    // Texture layer whose image the warp reads, or -1 when it needs none. The layer's image has to
    // be on the GPU for the warp to read it, even when that layer is painting nothing itself.
    constexpr int warpSourceLayer(const ShdWarpAttribute &warp) {
        return warp.enabled && warp.source != ShdWarpSource::NOISE
                   ? static_cast<int>(warp.source) - 1
                   : -1;
    }
}
