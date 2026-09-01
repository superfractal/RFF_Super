//
// Created by Opus 5 on 2026-08-05
// Modified by Opus 5 on 2026-08-13, 2026-08-27
//

#pragma once
#include <cstdint>
#include <string>

#include "ShdTextureBlendMode.h"
#include "ShdTextureUVMode.h"

namespace merutilm::rff2 {
    // Stacked image texture layers, blended in index order over the escaping region. Fixed rather
    // than growable: each layer costs its own combined-image-sampler binding in the texture set,
    // and the video compute pipeline has no room for a set of its own.
    constexpr uint32_t TEXTURE_LAYER_COUNT = 4;

    // Image texture painted over the escaping (non-black) region only; the interior keeps mandelbrotColor.
    struct ShdTextureAttribute {
        bool enabled = false;
        // Source image on disk. Empty (or unreadable) disables sampling regardless of "enabled".
        std::string path = {};
        ShdTextureUVMode uvMode = ShdTextureUVMode::CYCLE_BAND;
        ShdTextureBlendMode blendMode = ShdTextureBlendMode::MULTIPLY;
        float opacity = 1.0f;
        // Texture repeats across one full palette cycle (u) and across the v source range.
        float scaleU = 1.0f;
        float scaleV = 1.0f;
        // Texture-space scroll in repeats per second; drives the texture's own animation.
        float scrollU = 0.0f;
        float scrollV = 0.0f;
        // Share of the palette's own animation that U inherits. 1 rides the colors exactly,
        // 0 holds the texture still while they animate, negative runs it against them.
        float paletteFollow = 1.0f;
        // Iterations spanned by one texture tile along U. 0 follows the palette's cycle length,
        // which ties the texture's size to the coloring; set it to size the texture on its own.
        float periodIterations = 0.0f;
        // Share of the cell one repeat spans that the image fills, centred in it. Resizes the picture without changing how many of it there are.
        float size = 1.0f;
        // Fits the image inside its cell at its own width:height instead of stretching it to fill the cell.
        bool keepAspect = true;
    };
}
