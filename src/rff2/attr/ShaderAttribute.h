// Modified by Opus 5 on 2026-08-05, 2026-08-07, 2026-08-13, 2026-08-15, 2026-08-17, 2026-08-19
#pragma once
#include <array>

#include "ShdBloomAttribute.h"
#include "ShdColorAttribute.h"
#include "ShdFogAttribute.h"
#include "ShdHdrAttribute.h"
#include "ShdPaletteAttribute.h"
#include "ShdPatternAttribute.h"
#include "ShdSlopeAttribute.h"
#include "ShdStripeAttribute.h"
#include "ShdTextureAttribute.h"
#include "ShdWarpAttribute.h"


namespace merutilm::rff2 {
    struct ShaderAttribute {
        ShdPaletteAttribute palette;
        ShdStripeAttribute stripe;
        ShdSlopeAttribute slope;
        ShdColorAttribute color;
        ShdFogAttribute fog;
        ShdBloomAttribute bloom;
        // Blended in index order, so [0] is the bottom layer. Index 0 is the one older presets and
        // configs carry; the rest are appended at the end of those files.
        std::array<ShdTextureAttribute, TEXTURE_LAYER_COUNT> textures;
        // Blended in index order after the texture stack, so [0] is the bottom pattern. Index 0 is
        // the one older presets and configs carry; the rest are appended at the end of those files.
        std::array<ShdPatternAttribute, PATTERN_LAYER_COUNT> patterns;
        ShdWarpAttribute warp;
        // Read by the final pass, after fog and bloom have had their say in linear light.
        ShdHdrAttribute hdr;
    };
}
