//
// Modified by GPT-6 on 2026-09-19, 2026-09-20, 2026-09-23
//

#pragma once
#include "ShaderAttribute.h"
#include <algorithm>

namespace merutilm::rff2 {
    inline bool applySurfaceStyleRecipe(ShaderAttribute& shader, ShdSurfaceStyle style) {
        if (int(style) < 0 || int(style) > 6) {
            return false;
        }

        auto& slope = shader.slope;
        auto& palette = shader.palette;
        const ShdSlopeAttribute defaults{};

        slope.surfaceStyle = style;
        slope.surfaceBlend = ShdSurfaceBlend::MIX;
        slope.chromeStrength = 1;
        slope.layerCommon = 0;
        slope.layerMetal = 0;
        slope.layerSigil = 0;
        slope.layerPhonk = 0;
        slope.layerFrost = 0;
        slope.layerSea = 0;
        slope.layerPrint = 0;
        slope.layerVhs = 0;
        slope.layerMono = 0;
        slope.layerQuantize = 0;
        slope.styleRimStrength = 0;

        palette.bandLineEnabled = false;
        palette.bandLineGroove = false;
        palette.bandLineCount = 16;
        palette.bandLineWidth = .012f;
        palette.bandLineOpacity = 1;
        palette.bandLineSoftness = .15f;
        palette.bandLineColor = {.006f, .006f, .006f, 1};
        palette.bandSpineAmount = 0;
        palette.bandSpineLength = 1;
        palette.bandSpineDensity = 1;
        palette.bandBranchAmount = 1;
        palette.bandOrnamentAmount = 0;
        palette.bandOrnamentSize = 1;
        palette.bandOrnamentDensity = 1;
        palette.bandOrnamentInset = .032f;

        if (style != ShdSurfaceStyle::ORIGINAL) {
            slope.studio.use = true;
            if (slope.depth == 0) {
                slope.depth = .004f;
            }
            if (slope.opacity == 0) {
                slope.opacity = 1;
            }
        }

        switch (style) {
            case ShdSurfaceStyle::LIQUID_METAL:
                palette.bandLineEnabled = true;
                palette.bandLineWidth = .008f;
                break;
            case ShdSurfaceStyle::CYBER_SIGILISM:
                palette.bandLineEnabled = true;
                palette.bandLineWidth = .016f;
                palette.bandSpineAmount = .7f;
                palette.bandOrnamentAmount = 1;
                break;
            case ShdSurfaceStyle::PHONK:
                palette.bandLineEnabled = true;
                slope.layerVhs = 1;
                slope.vhsNoise = defaults.vhsNoise;
                slope.chromaticShift = defaults.chromaticShift;
                slope.pixelMix = defaults.pixelMix;
                slope.pixelSize = defaults.pixelSize;
                slope.filmGrain = defaults.filmGrain;
                slope.grungeScale = defaults.grungeScale;
                break;
            case ShdSurfaceStyle::BLACK_METAL:
                palette.bandLineEnabled = true;
                palette.bandLineWidth = .006f;
                palette.bandLineColor = {1, 1, 1, 1};
                palette.bandSpineAmount = .6f;
                palette.bandSpineLength = .8f;
                slope.layerMono = 1;
                slope.styleMonochrome = 1;
                slope.filmGrain = defaults.filmGrain;
                slope.grungeScale = defaults.grungeScale;
                break;
            case ShdSurfaceStyle::UKIYO_E:
                palette.bandLineEnabled = true;
                palette.bandLineColor = slope.printInk;
                slope.layerQuantize = 1;
                slope.ukiyoFlatness = 1;
                break;
            default:
                break;
        }

        if (shader.layerOrder.enabled) {
            for (auto layer : {
                     ShdLayer::SURFACE,
                     ShdLayer::BAND_LINE,
                     ShdLayer::VHS,
                     ShdLayer::MONO,
                     ShdLayer::PRINT_FINISH,
                 }) {
                shader.layerOrder.setVisible(layer, true);
            }

            if (palette.bandLineEnabled) {
                auto& order = shader.layerOrder;
                uint32_t from = 0;
                uint32_t last = 0;
                for (uint32_t i = 0; i < order.COUNT; ++i) {
                    if (order.layers[i] == ShdLayer::BAND_LINE) {
                        from = i;
                    }
                    if (uint32_t(order.layers[i]) >= 11 && uint32_t(order.layers[i]) <= 18) {
                        last = i;
                    }
                }
                if (from < last) {
                    order.move(from, last);
                }
            }
        }
        return true;
    }
}
