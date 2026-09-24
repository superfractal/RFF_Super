//
// Modified by GPT-6 on 2026-09-13, 2026-09-18, 2026-09-19, 2026-09-20, 2026-09-22, 2026-09-24
//

#pragma once
#include "ShdSlopeAttribute.h"

namespace merutilm::rff2 {
    inline int surfaceControlGroup(const ShdSlopeAttribute &slope, const float *changedField) {
        if (changedField == &slope.chromeStrength || changedField == &slope.paletteColorMix ||
            changedField == &slope.styleColorMix || changedField == &slope.styleHighlightMix ||
            changedField == &slope.styleBackgroundMix || changedField == &slope.styleRimStrength ||
            changedField == &slope.styleRimWidth || changedField == &slope.styleDetailLight ||
            changedField == &slope.styleDetailSuppress || changedField == &slope.styleDetailThreshold ||
            changedField == &slope.surfacePhase) {
            return 1;
        }
        if (changedField == &slope.filmStrength || changedField == &slope.reflectionDetail ||
            changedField == &slope.reflectionContrast || changedField == &slope.reflectionBrightness ||
            changedField == &slope.reflectionCurve || changedField == &slope.prismWidth ||
            changedField == &slope.prismSpread || changedField == &slope.filmHue ||
            changedField == &slope.sparkleStrength || changedField == &slope.styleGlintSize) {
            return 2;
        }
        if (changedField == &slope.backgroundBrightness || changedField == &slope.styleInkPreserve) {
            return 3;
        }
        if (changedField == &slope.shadowCrush || changedField == &slope.flameStrength ||
            changedField == &slope.phonkRed || changedField == &slope.styleDamage) {
            return 4;
        }
        if (changedField == &slope.frostStrength || changedField == &slope.frostThreshold) {
            return 5;
        }
        if (changedField == &slope.seaGlow || changedField == &slope.seaThreshold ||
            changedField == &slope.seaBody || changedField == &slope.seaParticles ||
            changedField == &slope.seaParticleSize || changedField == &slope.seaBalance) {
            return 6;
        }
        if (changedField == &slope.ukiyoFoam || changedField == &slope.ukiyoGrain ||
            changedField == &slope.ukiyoFlatness || changedField == &slope.ukiyoBalance) {
            return 7;
        }
        if (changedField == &slope.ukiyoColors) {
            return 8;
        }
        if (changedField == &slope.vhsNoise || changedField == &slope.chromaticShift ||
            changedField == &slope.pixelMix || changedField == &slope.pixelSize ||
            changedField == &slope.filmGrain || changedField == &slope.grungeScale) {
            return 9;
        }
        if (changedField == &slope.styleMonochrome) {
            return 10;
        }
        return 0;
    }

    inline void activateSurfaceControl(ShdSlopeAttribute &slope, const float *changedField) {
        if (changedField == &slope.studioEnvironmentRotation ||
            changedField == &slope.studioEnvironmentFollow) {
            slope.studio.use = true;
            return;
        }
        if (changedField == &slope.layerVhs || changedField == &slope.layerMono ||
            changedField == &slope.layerQuantize || changedField == &slope.vhsNoise ||
            changedField == &slope.chromaticShift || changedField == &slope.pixelMix ||
            changedField == &slope.pixelSize || changedField == &slope.filmGrain ||
            changedField == &slope.grungeScale || changedField == &slope.styleMonochrome ||
            changedField == &slope.ukiyoColors || changedField == &slope.ukiyoFlatness) {
            return;
        }
        const int controlGroup = surfaceControlGroup(slope, changedField);
        if (controlGroup == 0) {
            return;
        }
        slope.studio.use = true;
        const auto enableWhenZero = [](float &amount, float initialAmount = 0.35f) {
            if (amount == 0.0f) {
                amount = initialAmount;
            }
        };
        const int baseStyle = static_cast<int>(slope.surfaceStyle);
        if (controlGroup == 1 && baseStyle == 0) {
            slope.layerCommon = 1.0f;
        }
        if (controlGroup == 2 && baseStyle != 1) {
            enableWhenZero(slope.layerMetal);
        }
        if (controlGroup == 3 && baseStyle != 2) {
            enableWhenZero(slope.layerSigil);
        }
        if (controlGroup == 4 && baseStyle != 3) {
            enableWhenZero(slope.layerPhonk);
        }
        if (controlGroup == 5 && baseStyle != 4) {
            enableWhenZero(slope.layerFrost);
        }
        if (controlGroup == 6 && baseStyle != 5) {
            enableWhenZero(slope.layerSea);
        }
        if (controlGroup == 7 && baseStyle != 6) {
            enableWhenZero(slope.layerPrint);
        }
        if (baseStyle == 0 &&
            (changedField == &slope.surfacePhase || changedField == &slope.chromeStrength)) {
            enableWhenZero(slope.layerMetal);
        }
        if (changedField == &slope.styleRimWidth) {
            enableWhenZero(slope.styleRimStrength, 1.0f);
        }
        if (changedField == &slope.styleDetailThreshold) {
            enableWhenZero(slope.styleDetailSuppress, 0.75f);
        }
    }

    inline void activateSurfaceColor(ShdSlopeAttribute &slope, glm::vec4 *changedColor) {
        const auto enableWhenZero = [](float &amount) {
            if (amount == 0.0f) {
                amount = 1.0f;
            }
        };

        if (changedColor == &slope.seaBodyColor || changedColor == &slope.seaGlowColor ||
            changedColor == &slope.seaAccentColor) {
            activateSurfaceControl(slope, &slope.seaGlow);
            return;
        }
        if (changedColor == &slope.printInk || changedColor == &slope.printIndigo ||
            changedColor == &slope.printAsagi || changedColor == &slope.printBlue ||
            changedColor == &slope.printFoam || changedColor == &slope.printPaper) {
            if (changedColor == &slope.printAsagi) {
                slope.ukiyoColors = std::max(slope.ukiyoColors, 4.0f);
            }
            if (changedColor == &slope.printBlue) {
                slope.ukiyoColors = std::max(slope.ukiyoColors, 5.0f);
            }
            if (changedColor == &slope.printFoam) {
                slope.ukiyoColors = 6.0f;
            }
            return;
        }
        if (changedColor == &slope.styleColor) {
            enableWhenZero(slope.styleColorMix);
        } else if (changedColor == &slope.styleHighlightColor) {
            enableWhenZero(slope.styleHighlightMix);
        } else if (changedColor == &slope.styleBackgroundColor) {
            enableWhenZero(slope.styleBackgroundMix);
        } else if (changedColor == &slope.styleRimColor) {
            enableWhenZero(slope.styleRimStrength);
        } else {
            return;
        }
        if (slope.surfaceStyle == ShdSurfaceStyle::BLACK_METAL) {
            slope.styleMonochrome = 0.0f;
        }
        if (slope.surfaceStyle == ShdSurfaceStyle::UKIYO_E) {
            slope.ukiyoFlatness = 0.0f;
        }
        activateSurfaceControl(slope, &slope.styleColorMix);
    }
} // namespace merutilm::rff2
