//
// Created and modified by AI; earlier exact dates unavailable.

//
// Modified by Opus 4.8 on 2026-07-05
// Modified by Opus 5 on 2026-08-05, 2026-08-07, 2026-08-08, 2026-08-13, 2026-08-14, 2026-08-15, 2026-08-16, 2026-08-17, 2026-08-19, 2026-08-20, 2026-08-22, 2026-08-24, 2026-08-27, 2026-08-29, 2026-08-31
// Modified by GPT-5 on 2026-08-16, 2026-08-21, 2026-08-23, 2026-08-27, 2026-08-31, 2026-09-01
// Modified by ox-alpha on 2026-08-22
// Modified by Fable 5.1 on 2026-09-02
// Modified by GPT-6 on 2026-09-10, 2026-09-11, 2026-09-12, 2026-09-13, 2026-09-14, 2026-09-16, 2026-09-17, 2026-09-18, 2026-09-19, 2026-09-20, 2026-09-21, 2026-09-23, 2026-09-24
//

#include "ShaderPresetIO.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <fstream>
#include <vector>

#include "../../vulkan_helper/core/logger.hpp"
#include "../ui/IOUtilities.h"
#include "../preset/shader/palette/ShdPalettePresets.h"

namespace merutilm::rff2 {
    namespace {
        void readRetiredFloats(std::ifstream &in, bool legacyLayout, uint32_t count) {
            if (!legacyLayout) return;
            for (uint32_t i = 0; i < count && !in.fail() && in.rdbuf()->sgetc() != std::char_traits<char>::eof(); ++i) {
                float discarded;
                IOUtilities::readAndDecode(in, &discarded);
            }
        }

        constexpr uint64_t MAX_PALETTE_COLORS = 1ULL << 20;
        constexpr uint64_t MAX_TEXTURE_PATH_BYTES = 1024ULL * 1024;

        template <typename T> bool finiteStored(T value) {
            if constexpr (sizeof(T) == 4) {
                return (std::bit_cast<uint32_t>(value) & 0x7f800000u) != 0x7f800000u;
            } else {
                return (std::bit_cast<uint64_t>(value) & 0x7ff0000000000000ULL) != 0x7ff0000000000000ULL;
            }
        }

        bool finiteVec4(const glm::vec4 &v) {
            return finiteStored(v.x) && finiteStored(v.y) && finiteStored(v.z) && finiteStored(v.w);
        }

        bool finiteInRange(const float value, const float minimum, const float maximum) {
            return finiteStored(value) && value >= minimum && value <= maximum;
        }

        template <typename E> bool enumInRange(const E value, const int32_t min, const int32_t max) {
            const int32_t raw = static_cast<int32_t>(value);
            return raw >= min && raw <= max;
        }
    } // namespace

    bool ShaderPresetIO::validate(const ShaderAttribute &shader) {
        if (!shader.layerOrder.valid()) {
            return false;
        }
        if (!finiteStored(shader.palette.bandSpineAmount) || shader.palette.bandSpineAmount < 0.0f ||
            shader.palette.bandSpineAmount > 1.0f) {
            return false;
        }
        if (!finiteStored(shader.palette.bandSpineLength) || shader.palette.bandSpineLength < 0.0f ||
            shader.palette.bandSpineLength > 3.0f) {
            return false;
        }
        if (!finiteStored(shader.palette.bandSpineDensity) || shader.palette.bandSpineDensity < 0.25f ||
            shader.palette.bandSpineDensity > 3.0f) {
            return false;
        }
        if (!finiteStored(shader.palette.bandBranchAmount) || shader.palette.bandBranchAmount < 0.0f ||
            shader.palette.bandBranchAmount > 1.0f) {
            return false;
        }
        if (!finiteStored(shader.palette.bandOrnamentAmount) || shader.palette.bandOrnamentAmount < 0.0f ||
            shader.palette.bandOrnamentAmount > 1.0f) {
            return false;
        }
        if (!finiteStored(shader.palette.bandOrnamentSize) || shader.palette.bandOrnamentSize < 0.25f ||
            shader.palette.bandOrnamentSize > 3.0f) {
            return false;
        }
        if (!finiteStored(shader.palette.bandOrnamentDensity) || shader.palette.bandOrnamentDensity < 0.25f ||
            shader.palette.bandOrnamentDensity > 2.0f) {
            return false;
        }
        if (!finiteStored(shader.palette.bandOrnamentInset) || shader.palette.bandOrnamentInset < 0.0f ||
            shader.palette.bandOrnamentInset > 0.25f) {
            return false;
        }

        const auto allFinite = [](const std::initializer_list<float> values) {
            return std::ranges::all_of(values, [](const float value) { return finiteStored(value); });
        };
        for (const auto &effect : shader.effects) {
            if (!enumInRange(effect.type, 0, 7) || !enumInRange(effect.blend, 0, 3) ||
                !enumInRange(effect.mask, 0, 4) || !enumInRange(effect.rainShape, 0, 1)) {
                return false;
            }
            if (!finiteInRange(effect.dropSize, 0.1f, 8.0f) ||
                !finiteInRange(effect.opacity, 0.0f, 1.0f) ||
                !finiteInRange(effect.scale, 0.1f, 100.0f) ||
                !finiteInRange(effect.speed, -10.0f, 10.0f) ||
                !finiteInRange(effect.evolution, -10.0f, 10.0f) ||
                !finiteInRange(effect.density, 0.0f, 1.0f) ||
                !finiteInRange(effect.length, 0.05f, 2.0f) ||
                !finiteInRange(effect.width, 0.1f, 10.0f) ||
                !finiteInRange(effect.direction, -180.0f, 180.0f) ||
                !finiteInRange(effect.distortion, 0.0f, 4.0f) ||
                !finiteInRange(effect.glow, 0.0f, 4.0f) ||
                !finiteInRange(effect.seed, 0.0f, 65535.0f) ||
                !finiteInRange(effect.period, 1.0f, 1e9f) ||
                !finiteVec4(effect.color) || !finiteVec4(effect.secondary)) {
                return false;
            }
            for (int j = 0; j < 4; ++j) {
                if (effect.color[j] < 0 || effect.color[j] > 1 || effect.secondary[j] < 0 ||
                    effect.secondary[j] > 1) {
                    return false;
                }
            }
        }
        const auto &palette = shader.palette;
        if (palette.colors.empty() || palette.colors.size() > MAX_PALETTE_COLORS ||
            !std::ranges::all_of(palette.colors, finiteVec4) || !finiteVec4(palette.iterationInterval) ||
            palette.iterationInterval.x <= 0.0f || palette.iterationInterval.y <= 0.0f ||
            palette.iterationInterval.z <= 0.0f || palette.iterationInterval.w <= 0.0f ||
            !allFinite({palette.offsetRatio, palette.animationSpeed, palette.animationFlowAmount,
                        palette.animationFlowScale, palette.animationFlowSpeed, palette.animationFlowSwirl,
                        palette.staticColorTolerance, palette.cycleBias, palette.bandLineWidth,
                        palette.bandLineOpacity, palette.bandLineSoftness, palette.grooveDepth,
                        palette.grooveWidth}) ||
            palette.grooveDepth < 0 || palette.grooveDepth > 5 || palette.grooveWidth < 0 ||
            palette.grooveWidth > 0.5f || !finiteVec4(palette.glossColor) ||
            !finiteVec4(palette.mandelbrotColor) || !finiteVec4(palette.bandLineColor) ||
            !std::ranges::all_of(palette.staticColorIterations,
                                 [](const double value) { return finiteStored(value); }) ||
            !enumInRange(palette.colorSmoothing, 0, 2) || !enumInRange(palette.colorInterpolation, 0, 2) ||
            !enumInRange(palette.cycleCurve, 0, 1) || !enumInRange(palette.iterationColoring, 0, 6) ||
            !(palette.animationMode == ShdPaletteAnimationMode::LINEAR ||
              palette.animationMode == ShdPaletteAnimationMode::PSYCHEDELIC ||
              palette.animationMode == ShdPaletteAnimationMode::BREATHING ||
              palette.animationMode == ShdPaletteAnimationMode::TURBULENCE)) {
            return false;
        }

        const auto &stripe = shader.stripe;
        if (!allFinite({stripe.firstInterval, stripe.secondInterval, stripe.opacity, stripe.offset,
                        stripe.animationSpeed}) ||
            !enumInRange(stripe.stripeType, 0, 3)) {
            return false;
        }

        const auto &slope = shader.slope;
        if (!std::isfinite(slope.reflectionDetail) || slope.reflectionDetail < 0.25f ||
            slope.reflectionDetail > 4.0f) {
            return false;
        }
        if (!std::isfinite(slope.reflectionContrast) || slope.reflectionContrast < 0.25f ||
            slope.reflectionContrast > 3.0f) {
            return false;
        }
        if (!std::isfinite(slope.reflectionBrightness) || slope.reflectionBrightness < 0.0f ||
            slope.reflectionBrightness > 3.0f) {
            return false;
        }
        if (!std::isfinite(slope.reflectionCurve) || slope.reflectionCurve < 0.1f ||
            slope.reflectionCurve > 3.0f) {
            return false;
        }
        if (!std::isfinite(slope.surfacePhase) || slope.surfacePhase < 0.0f || slope.surfacePhase > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.prismWidth) || slope.prismWidth < 0.1f || slope.prismWidth > 3.0f) {
            return false;
        }
        if (!std::isfinite(slope.prismSpread) || slope.prismSpread < 0.0f || slope.prismSpread > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.filmHue) || slope.filmHue < 0.0f || slope.filmHue > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.sparkleStrength) || slope.sparkleStrength < 0.0f ||
            slope.sparkleStrength > 3.0f) {
            return false;
        }
        if (!std::isfinite(slope.backgroundBrightness) || slope.backgroundBrightness < 0.0f ||
            slope.backgroundBrightness > 8.0f) {
            return false;
        }
        if (!std::isfinite(slope.shadowCrush) || slope.shadowCrush < 0.0f || slope.shadowCrush > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.flameStrength) || slope.flameStrength < 0.0f || slope.flameStrength > 3.0f) {
            return false;
        }
        if (!std::isfinite(slope.phonkRed) || slope.phonkRed < 0.0f || slope.phonkRed > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.vhsNoise) || slope.vhsNoise < 0.0f || slope.vhsNoise > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.chromaticShift) || slope.chromaticShift < 0.0f ||
            slope.chromaticShift > 8.0f) {
            return false;
        }
        if (!std::isfinite(slope.pixelMix) || slope.pixelMix < 0.0f || slope.pixelMix > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.pixelSize) || slope.pixelSize < 1.0f || slope.pixelSize > 16.0f) {
            return false;
        }
        if (!std::isfinite(slope.filmGrain) || slope.filmGrain < 0.0f || slope.filmGrain > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.frostStrength) || slope.frostStrength < 0.0f || slope.frostStrength > 3.0f) {
            return false;
        }
        if (!std::isfinite(slope.frostThreshold) || slope.frostThreshold < 0.0f ||
            slope.frostThreshold > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.grungeScale) || slope.grungeScale < 0.25f || slope.grungeScale > 4.0f) {
            return false;
        }
        if (!std::isfinite(slope.paletteColorMix) || slope.paletteColorMix < 0.0f ||
            slope.paletteColorMix > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.styleColorMix) || slope.styleColorMix < 0.0f || slope.styleColorMix > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.styleHighlightMix) || slope.styleHighlightMix < 0.0f ||
            slope.styleHighlightMix > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.styleBackgroundMix) || slope.styleBackgroundMix < 0.0f ||
            slope.styleBackgroundMix > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.styleRimStrength) || slope.styleRimStrength < 0.0f ||
            slope.styleRimStrength > 4.0f) {
            return false;
        }
        if (!std::isfinite(slope.styleRimWidth) || slope.styleRimWidth < 0.1f ||
            slope.styleRimWidth > 32.0f) {
            return false;
        }
        if (!std::isfinite(slope.styleInkPreserve) || slope.styleInkPreserve < 0.0f ||
            slope.styleInkPreserve > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.styleMonochrome) || slope.styleMonochrome < 0.0f ||
            slope.styleMonochrome > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.styleGlintSize) || slope.styleGlintSize < 0.25f ||
            slope.styleGlintSize > 4.0f) {
            return false;
        }
        if (!std::isfinite(slope.styleDetailLight) || slope.styleDetailLight < 0.0f ||
            slope.styleDetailLight > 2.0f) {
            return false;
        }
        if (!std::isfinite(slope.styleDetailSuppress) || slope.styleDetailSuppress < 0.0f ||
            slope.styleDetailSuppress > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.styleDetailThreshold) || slope.styleDetailThreshold < 0.005f ||
            slope.styleDetailThreshold > 0.4f) {
            return false;
        }
        if (!std::isfinite(slope.styleDamage) || slope.styleDamage < 0.0f || slope.styleDamage > 1.0f) {
            return false;
        }
        if (!enumInRange(slope.surfaceBlend, 0, 3)) {
            return false;
        }
        for (int i = 0; i < 3; ++i) {
            if (!std::isfinite(slope.styleColor[i]) || slope.styleColor[i] < 0 || slope.styleColor[i] > 1) {
                return false;
            }
        }
        for (int i = 0; i < 3; ++i) {
            if (!std::isfinite(slope.styleHighlightColor[i]) || slope.styleHighlightColor[i] < 0 ||
                slope.styleHighlightColor[i] > 1) {
                return false;
            }
        }
        for (int i = 0; i < 3; ++i) {
        }
        for (int i = 0; i < 3; ++i) {
            if (!std::isfinite(slope.styleBackgroundColor[i]) || slope.styleBackgroundColor[i] < 0 ||
                slope.styleBackgroundColor[i] > 1) {
                return false;
            }
        }
        for (int i = 0; i < 3; ++i) {
            if (!std::isfinite(slope.styleRimColor[i]) || slope.styleRimColor[i] < 0 ||
                slope.styleRimColor[i] > 1) {
                return false;
            }
        }
        if (!std::isfinite(slope.seaGlow) || slope.seaGlow < 0.0f || slope.seaGlow > 4.0f) {
            return false;
        }
        if (!std::isfinite(slope.seaThreshold) || slope.seaThreshold < 0.0f || slope.seaThreshold > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.seaBody) || slope.seaBody < 0.0f || slope.seaBody > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.seaParticles) || slope.seaParticles < 0.0f || slope.seaParticles > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.seaParticleSize) || slope.seaParticleSize < 0.25f ||
            slope.seaParticleSize > 3.0f) {
            return false;
        }
        if (!std::isfinite(slope.seaBalance) || slope.seaBalance < 0.0f || slope.seaBalance > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.ukiyoColors) || slope.ukiyoColors < 3.0f || slope.ukiyoColors > 6.0f) {
            return false;
        }
        if (!std::isfinite(slope.ukiyoFoam) || slope.ukiyoFoam < 0.0f || slope.ukiyoFoam > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.ukiyoGrain) || slope.ukiyoGrain < 0.0f || slope.ukiyoGrain > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.ukiyoFlatness) || slope.ukiyoFlatness < 0.0f || slope.ukiyoFlatness > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.ukiyoBalance) || slope.ukiyoBalance < 0.0f || slope.ukiyoBalance > 1.0f) {
            return false;
        }
        for (int i = 0; i < 3; ++i) {
            if (!std::isfinite(slope.seaBodyColor[i]) || slope.seaBodyColor[i] < 0 ||
                slope.seaBodyColor[i] > 1) {
                return false;
            }
        }
        for (int i = 0; i < 3; ++i) {
            if (!std::isfinite(slope.seaGlowColor[i]) || slope.seaGlowColor[i] < 0 ||
                slope.seaGlowColor[i] > 1) {
                return false;
            }
        }
        for (int i = 0; i < 3; ++i) {
            if (!std::isfinite(slope.seaAccentColor[i]) || slope.seaAccentColor[i] < 0 ||
                slope.seaAccentColor[i] > 1) {
                return false;
            }
        }
        for (int i = 0; i < 3; ++i) {
            if (!std::isfinite(slope.printInk[i]) || slope.printInk[i] < 0 || slope.printInk[i] > 1) {
                return false;
            }
        }
        for (int i = 0; i < 3; ++i) {
            if (!std::isfinite(slope.printIndigo[i]) || slope.printIndigo[i] < 0 ||
                slope.printIndigo[i] > 1) {
                return false;
            }
        }
        for (int i = 0; i < 3; ++i) {
            if (!std::isfinite(slope.printAsagi[i]) || slope.printAsagi[i] < 0 || slope.printAsagi[i] > 1) {
                return false;
            }
        }
        for (int i = 0; i < 3; ++i) {
            if (!std::isfinite(slope.printBlue[i]) || slope.printBlue[i] < 0 || slope.printBlue[i] > 1) {
                return false;
            }
        }
        for (int i = 0; i < 3; ++i) {
            if (!std::isfinite(slope.printFoam[i]) || slope.printFoam[i] < 0 || slope.printFoam[i] > 1) {
                return false;
            }
        }
        for (int i = 0; i < 3; ++i) {
            if (!std::isfinite(slope.printPaper[i]) || slope.printPaper[i] < 0 || slope.printPaper[i] > 1) {
                return false;
            }
        }
        if (!std::isfinite(slope.layerMetal) || slope.layerMetal < 0.0f || slope.layerMetal > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.layerSigil) || slope.layerSigil < 0.0f || slope.layerSigil > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.layerPhonk) || slope.layerPhonk < 0.0f || slope.layerPhonk > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.layerFrost) || slope.layerFrost < 0.0f || slope.layerFrost > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.layerSea) || slope.layerSea < 0.0f || slope.layerSea > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.layerPrint) || slope.layerPrint < 0.0f || slope.layerPrint > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.layerQuantize) || slope.layerQuantize < 0.0f || slope.layerQuantize > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.layerVhs) || slope.layerVhs < 0.0f || slope.layerVhs > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.layerMono) || slope.layerMono < 0.0f || slope.layerMono > 1.0f) {
            return false;
        }
        if (!std::isfinite(slope.layerCommon) || slope.layerCommon < 0.0f || slope.layerCommon > 1.0f) {
            return false;
        }
        if (!enumInRange(slope.surfaceStyle, 0, 6) ||
            !allFinite({slope.chromeStrength, slope.filmStrength}) ||
            slope.chromeStrength < 0 || slope.chromeStrength > 1 || slope.filmStrength < 0 ||
            slope.filmStrength > 1) {
            return false;
        }
        if (!allFinite({slope.depth,
                        slope.reflectionRatio,
                        slope.opacity,
                        slope.zenith,
                        slope.azimuth,
                        slope.specularIntensity,
                        slope.specularPower,
                        slope.rimIntensity,
                        slope.rimPower,
                        slope.brightness,
                        slope.gamma,
                        slope.aoIntensity,
                        slope.ambientIntensity,
                        slope.specularZenith,
                        slope.specularAzimuth,
                        slope.specularAnisotropy,
                        slope.specularAnisotropyAngle,
                        slope.macroRelief,
                        slope.macroRadius,
                        slope.reliefResponse,
                        slope.terminatorSoftness,
                        slope.highlightKnee,
                        slope.lumaAmount,
                        slope.tintResponse,
                        slope.shadowChroma,
                        slope.fillIntensity,
                        slope.fillZenith,
                        slope.fillAzimuth,
                        slope.glossIntensity,
                        slope.glossBands,
                        slope.glossSharpness,
                        slope.glossPhase,
                        slope.glossRelief}) ||
            !finiteVec4(slope.rimColor) || !finiteVec4(slope.specularColor) || !finiteVec4(slope.skyColor) ||
            !finiteVec4(slope.groundColor) || !finiteVec4(slope.glossColor) ||
            !(slope.shadingBlend == ShdSlopeShadingBlend::OVERLAY ||
              slope.shadingBlend == ShdSlopeShadingBlend::OKLAB_LIGHTNESS) ||
            !enumInRange(slope.lightBlend, 0, 1) || !enumInRange(slope.tintBlend, 0, 1) ||
            !enumInRange(slope.glossSource, 0, 3)) {
            return false;
        }

        if (!finiteStored(slope.studioEnvironmentRotation) || slope.studioEnvironmentRotation < -360000.0f ||
            slope.studioEnvironmentRotation > 360000.0f) {
            return false;
        }
        if (!finiteStored(slope.studioEnvironmentFollow) || slope.studioEnvironmentFollow < 0.0f ||
            slope.studioEnvironmentFollow > 1.0f) {
            return false;
        }
        const auto &studio = slope.studio;
        if (!allFinite({slope.reliefZoomReference}) ||
            (slope.reliefZoomReference < 0.0f && slope.reliefZoomReference != -1.0f)) {
            return false;
        }
        if (!validPaletteStops(palette)) {
            return false;
        }
        if (!allFinite({slope.iridescence, slope.filmThickness, slope.specularAA, slope.reliefDepth,
                        slope.normalSmooth, slope.aoRadius, slope.reliefWaves, slope.waveFrequency,
                        slope.boundaryGuard}) ||
            slope.iridescence < 0 || slope.iridescence > 1 || slope.filmThickness < 0 ||
            slope.filmThickness > 2000 || slope.specularAA < 0 || slope.specularAA > 1 ||
            slope.reliefDepth < 0 || slope.reliefDepth > 16 || slope.normalSmooth < 0 ||
            slope.normalSmooth > 1 || slope.aoRadius < 1 || slope.aoRadius > 64 || slope.reliefWaves < 0 ||
            slope.reliefWaves > 1 || slope.waveFrequency < 0.03f || slope.waveFrequency > 1.5f ||
            slope.boundaryGuard < 0 || slope.boundaryGuard > 1) {
            return false;
        }
        if (!allFinite({studio.roughness, studio.metalness, studio.ior, studio.directIntensity,
                        studio.environmentIntensity, studio.clearcoat,
                        studio.clearcoatRoughness}) ||
            studio.roughness < 0.04f || studio.roughness > 1.0f || studio.metalness < 0.0f ||
            studio.metalness > 1.0f || studio.ior < 1.0f || studio.ior > 3.0f ||
            studio.directIntensity < 0.0f || studio.directIntensity > 8.0f ||
            studio.environmentIntensity < 0.0f || studio.environmentIntensity > 8.0f ||
            studio.clearcoat < 0.0f ||
            studio.clearcoat > 1.0f || studio.clearcoatRoughness < 0.04f ||
            studio.clearcoatRoughness > 1.0f) {
            return false;
        }

        const auto &color = shader.color;
        const auto &fog = shader.fog;
        if (!allFinite({fog.chaosAmount, fog.chaosScale, fog.chaosThreshold, fog.chaosTransition,
                        fog.chaosFeather, fog.chaosBlur, fog.chaosHighlights, fog.chaosShade}) ||
            fog.chaosAmount < 0.0f || fog.chaosAmount > 1.0f || fog.chaosScale < 0.5f ||
            fog.chaosScale > 4.0f || fog.chaosThreshold < 0.0f || fog.chaosThreshold > 1.0f ||
            fog.chaosTransition < 0.01f || fog.chaosTransition > 1.0f || fog.chaosFeather < 0.0f ||
            fog.chaosFeather > 32.0f || fog.chaosBlur < 0.0f || fog.chaosBlur > 32.0f ||
            fog.chaosHighlights < 0.0f || fog.chaosHighlights > 1.0f || fog.chaosShade < 0.0f ||
            fog.chaosShade > 0.5f) {
            return false;
        }
        const auto &bloom = shader.bloom;
        if (!allFinite({color.gamma,      color.exposure, color.hue,       color.saturation, color.brightness,
                        color.contrast,   fog.radius,     fog.opacity,     fog.centerStart,  fog.rimMask,
                        fog.rimMaskBoost, fog.rimBlur,    fog.focusAmount, fog.focusRatio,   fog.focusRange,
                        fog.focusFalloff, fog.focusBlur,  bloom.threshold, bloom.radius,     bloom.softness,
                        bloom.intensity}) ||
            !enumInRange(fog.blurQuality, 0, 1)) {
            return false;
        }

        for (const auto &texture : shader.textures) {
            if (!allFinite({texture.opacity, texture.scaleU, texture.scaleV, texture.scrollU, texture.scrollV,
                            texture.paletteFollow, texture.periodIterations, texture.size}) ||
                !enumInRange(texture.uvMode, 0, 3) || !enumInRange(texture.blendMode, 0, 2)) {
                return false;
            }
        }
        for (const auto &pattern : shader.patterns) {
            if (!allFinite({pattern.opacity, pattern.paletteShift, pattern.sharpness, pattern.scaleU,
                            pattern.scaleV, pattern.scrollU, pattern.scrollV, pattern.paletteFollow,
                            pattern.periodIterations, pattern.edgeWidth, pattern.edgeOpacity}) ||
                !finiteVec4(pattern.color) || !finiteVec4(pattern.edgeColor) ||
                !enumInRange(pattern.type, 0, 7) || !enumInRange(pattern.uvMode, 0, 3) ||
                !enumInRange(pattern.blendMode, 0, 2) || !enumInRange(pattern.inkMode, 0, 1)) {
                return false;
            }
        }

        const auto &warp = shader.warp;
        const auto &hdr = shader.hdr;
        return allFinite({warp.amount, warp.octaves, warp.scaleU, warp.scaleV, warp.scrollU, warp.scrollV,
                          warp.paletteFollow, warp.periodIterations, hdr.exposure, hdr.headroom}) &&
               enumInRange(warp.source, 0, 4) && enumInRange(warp.uvMode, 0, 3) &&
               enumInRange(hdr.method, 0, 7) && std::isfinite(hdr.mfrPeakNits) && hdr.mfrPeakNits >= 100.0f &&
               hdr.mfrPeakNits <= 10000.0f;
    }

    void ShaderPresetIO::writeShader(std::ostream &out, const ShaderAttribute &shader) {
        auto writeVec4 = [&out](const glm::vec4 &v) {
            IOUtilities::encodeAndWrite(out, v.x);
            IOUtilities::encodeAndWrite(out, v.y);
            IOUtilities::encodeAndWrite(out, v.z);
            IOUtilities::encodeAndWrite(out, v.w);
        };

        // Palette (hybrid): a recipe palette stores only {id, seed}; otherwise the raw colors
        // are written as RGB float32 (alpha is always 1.0, so it is dropped and restored on load).
        const auto &p = shader.palette;
        IOUtilities::encodeAndWrite(out, p.recipePresetId);
        if (p.recipePresetId >= 0) {
            IOUtilities::encodeAndWrite(out, p.recipeSeed);
        } else {
            IOUtilities::encodeAndWrite(out, static_cast<uint64_t>(p.colors.size()));
            for (const auto &c : p.colors) {
                IOUtilities::encodeAndWrite(out, c.x);
                IOUtilities::encodeAndWrite(out, c.y);
                IOUtilities::encodeAndWrite(out, c.z);
            }
        }
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(p.colorSmoothing));
        writeVec4(p.iterationInterval);
        IOUtilities::encodeAndWrite(out, p.offsetRatio);
        IOUtilities::encodeAndWrite(out, p.animationSpeed);
        IOUtilities::encodeAndWrite(out, p.enableGloss);
        writeVec4(p.glossColor);
        IOUtilities::encodeAndWrite(out, p.seamless);
        writeVec4(p.mandelbrotColor);
        IOUtilities::encodeAndWrite(out, p.staticColorTolerance);
        IOUtilities::encodeAndWrite(out, static_cast<uint64_t>(p.staticColorIterations.size()));
        IOUtilities::encodeAndWrite(out, p.staticColorIterations);

        // Stripe
        const auto &st = shader.stripe;
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(st.stripeType));
        IOUtilities::encodeAndWrite(out, st.firstInterval);
        IOUtilities::encodeAndWrite(out, st.secondInterval);
        IOUtilities::encodeAndWrite(out, st.opacity);
        IOUtilities::encodeAndWrite(out, st.offset);
        IOUtilities::encodeAndWrite(out, st.animationSpeed);

        // Slope
        const auto &sl = shader.slope;
        IOUtilities::encodeAndWrite(out, sl.depth);
        IOUtilities::encodeAndWrite(out, sl.reflectionRatio);
        IOUtilities::encodeAndWrite(out, sl.opacity);
        IOUtilities::encodeAndWrite(out, sl.zenith);
        IOUtilities::encodeAndWrite(out, sl.azimuth);
        IOUtilities::encodeAndWrite(out, sl.specularIntensity);
        IOUtilities::encodeAndWrite(out, sl.specularPower);
        IOUtilities::encodeAndWrite(out, sl.rimIntensity);
        IOUtilities::encodeAndWrite(out, sl.rimPower);
        IOUtilities::encodeAndWrite(out, sl.brightness);
        IOUtilities::encodeAndWrite(out, sl.gamma);
        writeVec4(sl.rimColor);
        writeVec4(sl.specularColor);
        IOUtilities::encodeAndWrite(out, sl.aoIntensity);
        IOUtilities::encodeAndWrite(out, sl.ambientIntensity);
        writeVec4(sl.skyColor);
        writeVec4(sl.groundColor);
        IOUtilities::encodeAndWrite(out, sl.specularIndependent);
        IOUtilities::encodeAndWrite(out, sl.specularZenith);
        IOUtilities::encodeAndWrite(out, sl.specularAzimuth);
        IOUtilities::encodeAndWrite(out, sl.specularAnisotropy);
        IOUtilities::encodeAndWrite(out, sl.specularAnisotropyAngle);

        // Color
        const auto &co = shader.color;
        IOUtilities::encodeAndWrite(out, co.gamma);
        IOUtilities::encodeAndWrite(out, co.exposure);
        IOUtilities::encodeAndWrite(out, co.hue);
        IOUtilities::encodeAndWrite(out, co.saturation);
        IOUtilities::encodeAndWrite(out, co.brightness);
        IOUtilities::encodeAndWrite(out, co.contrast);

        // Fog
        const auto &fo = shader.fog;
        IOUtilities::encodeAndWrite(out, fo.radius);
        IOUtilities::encodeAndWrite(out, fo.opacity);

        // Bloom
        const auto &bl = shader.bloom;
        IOUtilities::encodeAndWrite(out, bl.threshold);
        IOUtilities::encodeAndWrite(out, bl.radius);
        IOUtilities::encodeAndWrite(out, bl.softness);
        IOUtilities::encodeAndWrite(out, bl.intensity);
    }

    void ShaderPresetIO::readShader(std::ifstream &in, ShaderAttribute &out, const bool newPaletteFormat,
                                    const bool hasFrozenColors, const bool legacyLayout) {
        auto readVec4 = [&in](glm::vec4 &v) {
            IOUtilities::readAndDecode(in, &v.x);
            IOUtilities::readAndDecode(in, &v.y);
            IOUtilities::readAndDecode(in, &v.z);
            IOUtilities::readAndDecode(in, &v.w);
        };

        ShaderAttribute &s = out;

        // Palette
        auto &p = s.palette;
        if (newPaletteFormat) {
            IOUtilities::readAndDecode(in, &p.recipePresetId);
            if (p.recipePresetId >= 0) {
                IOUtilities::readAndDecode(in, &p.recipeSeed);
                p.colors = ShdPalettePresets::regenerateRecipeColors(p.recipePresetId, p.recipeSeed);
            } else {
                p.recipeSeed = 0;
                uint64_t colorCount = 0;
                IOUtilities::readAndDecode(in, &colorCount);
                if (!IOUtilities::validateReadCount(in, colorCount, sizeof(float) * 3, MAX_PALETTE_COLORS)) {
                    return;
                }
                p.colors.resize(colorCount);
                for (uint64_t i = 0; i < colorCount; ++i) {
                    IOUtilities::readAndDecode(in, &p.colors[i].x);
                    IOUtilities::readAndDecode(in, &p.colors[i].y);
                    IOUtilities::readAndDecode(in, &p.colors[i].z);
                    p.colors[i].w = 1.0f;
                }
            }
        } else {
            // Legacy: full RGBA color array, no recipe.
            p.recipePresetId = -1;
            p.recipeSeed = 0;
            uint64_t colorCount = 0;
            IOUtilities::readAndDecode(in, &colorCount);
            if (!IOUtilities::validateReadCount(in, colorCount, sizeof(float) * 4, MAX_PALETTE_COLORS)) {
                return;
            }
            p.colors.resize(colorCount);
            for (uint64_t i = 0; i < colorCount; ++i) {
                readVec4(p.colors[i]);
            }
        }
        if (p.colors.empty()) {
            in.setstate(std::ios::failbit);
            return;
        }
        int32_t colorSmoothing;
        IOUtilities::readAndDecode(in, &colorSmoothing);
        p.colorSmoothing = static_cast<ShdPalColorSmoothingMethod>(colorSmoothing);
        readVec4(p.iterationInterval);
        IOUtilities::readAndDecode(in, &p.offsetRatio);
        IOUtilities::readAndDecode(in, &p.animationSpeed);
        p.animationMode = ShdPaletteAnimationMode::LINEAR;
        p.animationFlowAmount = 80.0f;
        p.animationFlowScale = 3.0f;
        p.animationFlowSpeed = 0.5f;
        p.animationFlowSwirl = 0.4f;
        IOUtilities::readAndDecode(in, &p.enableGloss);
        readVec4(p.glossColor);
        IOUtilities::readAndDecode(in, &p.seamless);
        readVec4(p.mandelbrotColor);
        if (hasFrozenColors) {
            IOUtilities::readAndDecode(in, &p.staticColorTolerance);
            uint64_t frozenCount;
            IOUtilities::readAndDecode(in, &frozenCount);
            if (!IOUtilities::validateReadCount(in, frozenCount, sizeof(double),
                                                ShdPaletteAttribute::MAX_STATIC_COLORS)) {
                return;
            }
            p.staticColorIterations.resize(frozenCount);
            IOUtilities::readAndDecode(in, &p.staticColorIterations);
        } else {
            p.staticColorTolerance = 0.02f;
            p.staticColorIterations.clear();
        }

        // Stripe
        auto &st = s.stripe;
        int32_t stripeType = 0;
        IOUtilities::readAndDecode(in, &stripeType);
        if (stripeType < static_cast<int32_t>(ShdStripeType::NONE) ||
            stripeType > static_cast<int32_t>(ShdStripeType::SQUARED)) {
            in.setstate(std::ios::failbit);
            return;
        }
        st.stripeType = static_cast<ShdStripeType>(stripeType);
        IOUtilities::readAndDecode(in, &st.firstInterval);
        IOUtilities::readAndDecode(in, &st.secondInterval);
        IOUtilities::readAndDecode(in, &st.opacity);
        IOUtilities::readAndDecode(in, &st.offset);
        IOUtilities::readAndDecode(in, &st.animationSpeed);

        // Slope
        auto &sl = s.slope;
        IOUtilities::readAndDecode(in, &sl.depth);
        IOUtilities::readAndDecode(in, &sl.reflectionRatio);
        IOUtilities::readAndDecode(in, &sl.opacity);
        IOUtilities::readAndDecode(in, &sl.zenith);
        IOUtilities::readAndDecode(in, &sl.azimuth);
        IOUtilities::readAndDecode(in, &sl.specularIntensity);
        IOUtilities::readAndDecode(in, &sl.specularPower);
        IOUtilities::readAndDecode(in, &sl.rimIntensity);
        IOUtilities::readAndDecode(in, &sl.rimPower);
        // Slope shadow was removed; its two floats are still consumed so older files keep their alignment.
        if (legacyLayout) {
            float removedSlopeShadow = 0.0f;
            IOUtilities::readAndDecode(in, &removedSlopeShadow);
            IOUtilities::readAndDecode(in, &removedSlopeShadow);
        }
        IOUtilities::readAndDecode(in, &sl.brightness);
        IOUtilities::readAndDecode(in, &sl.gamma);
        readVec4(sl.rimColor);
        readVec4(sl.specularColor);
        IOUtilities::readAndDecode(in, &sl.aoIntensity);
        IOUtilities::readAndDecode(in, &sl.ambientIntensity);
        readVec4(sl.skyColor);
        readVec4(sl.groundColor);
        IOUtilities::readAndDecode(in, &sl.specularIndependent);
        IOUtilities::readAndDecode(in, &sl.specularZenith);
        IOUtilities::readAndDecode(in, &sl.specularAzimuth);
        IOUtilities::readAndDecode(in, &sl.specularAnisotropy);
        IOUtilities::readAndDecode(in, &sl.specularAnisotropyAngle);

        // Color
        auto &co = s.color;
        IOUtilities::readAndDecode(in, &co.gamma);
        IOUtilities::readAndDecode(in, &co.exposure);
        IOUtilities::readAndDecode(in, &co.hue);
        IOUtilities::readAndDecode(in, &co.saturation);
        IOUtilities::readAndDecode(in, &co.brightness);
        IOUtilities::readAndDecode(in, &co.contrast);

        // Fog
        auto &fo = s.fog;
        IOUtilities::readAndDecode(in, &fo.radius);
        IOUtilities::readAndDecode(in, &fo.opacity);

        // Bloom
        auto &bl = s.bloom;
        IOUtilities::readAndDecode(in, &bl.threshold);
        IOUtilities::readAndDecode(in, &bl.radius);
        IOUtilities::readAndDecode(in, &bl.softness);
        IOUtilities::readAndDecode(in, &bl.intensity);
    }

    void ShaderPresetIO::writeAnimationShape(std::ostream &out, const ShaderAttribute &shader) {
        const auto &p = shader.palette;
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(p.animationMode));
        IOUtilities::encodeAndWrite(out, p.animationFlowAmount);
        IOUtilities::encodeAndWrite(out, p.animationFlowScale);
        IOUtilities::encodeAndWrite(out, p.animationFlowSpeed);
        IOUtilities::encodeAndWrite(out, p.animationFlowSwirl);
    }

    void ShaderPresetIO::readAnimationShape(std::ifstream &in, ShaderAttribute &out) {
        auto &p = out.palette;
        int32_t animationMode;
        IOUtilities::readAndDecode(in, &animationMode);
        p.animationMode = static_cast<ShdPaletteAnimationMode>(animationMode);
        IOUtilities::readAndDecode(in, &p.animationFlowAmount);
        IOUtilities::readAndDecode(in, &p.animationFlowScale);
        IOUtilities::readAndDecode(in, &p.animationFlowSpeed);
        IOUtilities::readAndDecode(in, &p.animationFlowSwirl);
    }

    void ShaderPresetIO::writeTexture(std::ostream &out, const ShaderAttribute &shader,
                                      const uint32_t layer) {
        const auto &t = shader.textures[layer];
        IOUtilities::encodeAndWrite(out, t.enabled);
        IOUtilities::encodeAndWrite(out, static_cast<uint64_t>(t.path.length()));
        IOUtilities::encodeAndWrite(out, t.path.data(), t.path.length());
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(t.uvMode));
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(t.blendMode));
        IOUtilities::encodeAndWrite(out, t.opacity);
        IOUtilities::encodeAndWrite(out, t.scaleU);
        IOUtilities::encodeAndWrite(out, t.scaleV);
        IOUtilities::encodeAndWrite(out, t.scrollU);
        IOUtilities::encodeAndWrite(out, t.scrollV);
        // Appended after the original texture block; each guarded on read.
        IOUtilities::encodeAndWrite(out, t.paletteFollow);
        IOUtilities::encodeAndWrite(out, t.periodIterations);
    }

    void ShaderPresetIO::readTexture(std::ifstream &in, ShaderAttribute &out, const uint32_t layer) {
        auto &t = out.textures[layer];
        IOUtilities::readAndDecode(in, &t.enabled);
        uint64_t len;
        IOUtilities::readAndDecode(in, &len);
        if (!IOUtilities::validateReadCount(in, len, sizeof(char), MAX_TEXTURE_PATH_BYTES)) {
            return;
        }
        t.path.resize(static_cast<size_t>(len));
        IOUtilities::readAndDecode(in, len, t.path.data());
        int32_t uvMode;
        IOUtilities::readAndDecode(in, &uvMode);
        // A file may name a mode this build no longer has; fall back rather than leave the radio
        // group with nothing selected.
        t.uvMode =
            uvMode >= 0 && uvMode <= 3 ? static_cast<ShdTextureUVMode>(uvMode) : ShdTextureUVMode::CYCLE_BAND;
        int32_t blendMode;
        IOUtilities::readAndDecode(in, &blendMode);
        t.blendMode = static_cast<ShdTextureBlendMode>(blendMode);
        IOUtilities::readAndDecode(in, &t.opacity);
        IOUtilities::readAndDecode(in, &t.scaleU);
        IOUtilities::readAndDecode(in, &t.scaleV);
        IOUtilities::readAndDecode(in, &t.scrollU);
        IOUtilities::readAndDecode(in, &t.scrollV);
        // Files written before these fields existed stop early and keep the defaults.
        auto hasMore = [&in] { return in.rdbuf()->sgetc() != std::char_traits<char>::eof(); };
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &t.paletteFollow);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &t.periodIterations);
        }
    }

    void ShaderPresetIO::writeTextureSize(std::ostream &out, const ShaderAttribute &shader) {
        for (uint32_t layer = 0; layer < TEXTURE_LAYER_COUNT; ++layer) {
            const auto &t = shader.textures[layer];
            IOUtilities::encodeAndWrite(out, t.size);
            IOUtilities::encodeAndWrite(out, t.keepAspect);
        }
    }

    void ShaderPresetIO::readTextureSize(std::ifstream &in, ShaderAttribute &out) {
        // Guarded per field, not just once: a file from a build with a shorter stack stops early.
        auto hasMore = [&in] { return in.rdbuf()->sgetc() != std::char_traits<char>::eof(); };
        for (uint32_t layer = 0; layer < TEXTURE_LAYER_COUNT; ++layer) {
            auto &t = out.textures[layer];
            if (hasMore()) {
                IOUtilities::readAndDecode(in, &t.size);
            }
            if (hasMore()) {
                IOUtilities::readAndDecode(in, &t.keepAspect);
            } else {
                t.keepAspect = false;
            }
        }
    }

    void ShaderPresetIO::clearLegacyTextureSize(ShaderAttribute &out) {
        for (uint32_t layer = 0; layer < TEXTURE_LAYER_COUNT; ++layer) {
            out.textures[layer].keepAspect = false;
        }
    }

    void ShaderPresetIO::writeExtraTextureLayers(std::ostream &out, const ShaderAttribute &shader) {
        for (uint32_t layer = 1; layer < TEXTURE_LAYER_COUNT; ++layer) {
            writeTexture(out, shader, layer);
        }
    }

    void ShaderPresetIO::readExtraTextureLayers(std::ifstream &in, ShaderAttribute &out) {
        // Guarded per layer, not just once: a file from a build with a shorter stack stops early and
        // leaves the layers it never wrote at their defaults.
        auto hasMore = [&in] { return in.rdbuf()->sgetc() != std::char_traits<char>::eof(); };
        for (uint32_t layer = 1; layer < TEXTURE_LAYER_COUNT && hasMore(); ++layer) {
            readTexture(in, out, layer);
        }
    }

    void ShaderPresetIO::writePattern(std::ostream &out, const ShaderAttribute &shader) {
        const auto &p = shader.patterns[0];
        IOUtilities::encodeAndWrite(out, p.enabled);
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(p.type));
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(p.uvMode));
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(p.blendMode));
        IOUtilities::encodeAndWrite(out, p.opacity);
        IOUtilities::encodeAndWrite(out, p.color.r);
        IOUtilities::encodeAndWrite(out, p.color.g);
        IOUtilities::encodeAndWrite(out, p.color.b);
        IOUtilities::encodeAndWrite(out, p.sharpness);
        IOUtilities::encodeAndWrite(out, p.scaleU);
        IOUtilities::encodeAndWrite(out, p.scaleV);
        IOUtilities::encodeAndWrite(out, p.scrollU);
        IOUtilities::encodeAndWrite(out, p.scrollV);
        IOUtilities::encodeAndWrite(out, p.paletteFollow);
        IOUtilities::encodeAndWrite(out, p.periodIterations);
        // Appended after the original pattern block; each guarded on read.
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(p.inkMode));
        IOUtilities::encodeAndWrite(out, p.paletteShift);
    }

    void ShaderPresetIO::readPattern(std::ifstream &in, ShaderAttribute &out) {
        auto &p = out.patterns[0];
        IOUtilities::readAndDecode(in, &p.enabled);
        int32_t type;
        IOUtilities::readAndDecode(in, &type);
        // A file may name a shape this build no longer has; fall back rather than leave the radio
        // group with nothing selected.
        p.type = type >= 0 && type <= 7 ? static_cast<ShdPatternType>(type) : ShdPatternType::STRIPES;
        int32_t uvMode;
        IOUtilities::readAndDecode(in, &uvMode);
        p.uvMode =
            uvMode >= 0 && uvMode <= 3 ? static_cast<ShdTextureUVMode>(uvMode) : ShdTextureUVMode::CYCLE_BAND;
        int32_t blendMode;
        IOUtilities::readAndDecode(in, &blendMode);
        p.blendMode = static_cast<ShdTextureBlendMode>(blendMode);
        IOUtilities::readAndDecode(in, &p.opacity);
        IOUtilities::readAndDecode(in, &p.color.r);
        IOUtilities::readAndDecode(in, &p.color.g);
        IOUtilities::readAndDecode(in, &p.color.b);
        IOUtilities::readAndDecode(in, &p.sharpness);
        IOUtilities::readAndDecode(in, &p.scaleU);
        IOUtilities::readAndDecode(in, &p.scaleV);
        IOUtilities::readAndDecode(in, &p.scrollU);
        IOUtilities::readAndDecode(in, &p.scrollV);
        IOUtilities::readAndDecode(in, &p.paletteFollow);
        IOUtilities::readAndDecode(in, &p.periodIterations);
        // Files written before these fields existed stop early and keep the defaults.
        auto hasMore = [&in] { return in.rdbuf()->sgetc() != std::char_traits<char>::eof(); };
        if (hasMore()) {
            int32_t inkMode;
            IOUtilities::readAndDecode(in, &inkMode);
            p.inkMode = inkMode >= 0 && inkMode <= 1 ? static_cast<ShdPatternInkMode>(inkMode)
                                                     : ShdPatternInkMode::PALETTE_SHIFT;
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &p.paletteShift);
        }
    }

    void ShaderPresetIO::writeOklabModes(std::ostream &out, const ShaderAttribute &shader) {
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(shader.palette.colorInterpolation));
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(shader.slope.shadingBlend));
    }

    void ShaderPresetIO::readOklabModes(std::ifstream &in, ShaderAttribute &out) {
        // Guarded one field at a time, so a file written between the two still loads.
        auto hasMore = [&in] { return in.rdbuf()->sgetc() != std::char_traits<char>::eof(); };
        if (hasMore()) {
            int32_t interpolation = 0;
            IOUtilities::readAndDecode(in, &interpolation);
            // A file may name a mode this build no longer has; fall back rather than leave the radio group empty.
            out.palette.colorInterpolation = interpolation == 1   ? ShdPalColorInterpolationMethod::OKLAB
                                             : interpolation == 2 ? ShdPalColorInterpolationMethod::LINEAR_RGB
                                                                  : ShdPalColorInterpolationMethod::RGB;
        }
        if (hasMore()) {
            int32_t shadingBlend;
            IOUtilities::readAndDecode(in, &shadingBlend);
            out.slope.shadingBlend =
                shadingBlend == 2 ? ShdSlopeShadingBlend::OKLAB_LIGHTNESS : ShdSlopeShadingBlend::OVERLAY;
        }
    }

    void ShaderPresetIO::writeWarp(std::ostream &out, const ShaderAttribute &shader) {
        const auto &w = shader.warp;
        IOUtilities::encodeAndWrite(out, w.enabled);
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(w.source));
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(w.uvMode));
        IOUtilities::encodeAndWrite(out, w.amount);
        IOUtilities::encodeAndWrite(out, w.octaves);
        IOUtilities::encodeAndWrite(out, w.scaleU);
        IOUtilities::encodeAndWrite(out, w.scaleV);
        IOUtilities::encodeAndWrite(out, w.scrollU);
        IOUtilities::encodeAndWrite(out, w.scrollV);
        IOUtilities::encodeAndWrite(out, w.paletteFollow);
        IOUtilities::encodeAndWrite(out, w.periodIterations);
    }

    void ShaderPresetIO::readWarp(std::ifstream &in, ShaderAttribute &out) {
        // Guarded one field at a time, so a file written between any two of them still loads.
        auto hasMore = [&in] { return in.rdbuf()->sgetc() != std::char_traits<char>::eof(); };
        auto &w = out.warp;
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &w.enabled);
        }
        if (hasMore()) {
            int32_t source;
            IOUtilities::readAndDecode(in, &source);
            // A file may name a source this build no longer has; fall back rather than read a layer that is not there.
            w.source = source >= 0 && source <= static_cast<int32_t>(TEXTURE_LAYER_COUNT)
                           ? static_cast<ShdWarpSource>(source)
                           : ShdWarpSource::NOISE;
        }
        if (hasMore()) {
            int32_t uvMode;
            IOUtilities::readAndDecode(in, &uvMode);
            w.uvMode = uvMode >= 0 && uvMode <= 3 ? static_cast<ShdTextureUVMode>(uvMode)
                                                  : ShdTextureUVMode::CYCLE_BAND;
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &w.amount);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &w.octaves);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &w.scaleU);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &w.scaleV);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &w.scrollU);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &w.scrollV);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &w.paletteFollow);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &w.periodIterations);
        }
    }

    void ShaderPresetIO::writePatternEdge(std::ostream &out, const ShaderAttribute &shader) {
        const auto &p = shader.patterns[0];
        IOUtilities::encodeAndWrite(out, p.edgeEnabled);
        IOUtilities::encodeAndWrite(out, p.edgeColor.r);
        IOUtilities::encodeAndWrite(out, p.edgeColor.g);
        IOUtilities::encodeAndWrite(out, p.edgeColor.b);
        IOUtilities::encodeAndWrite(out, p.edgeWidth);
        IOUtilities::encodeAndWrite(out, p.edgeOpacity);
    }

    void ShaderPresetIO::readPatternEdge(std::ifstream &in, ShaderAttribute &out) {
        // Guarded one field at a time, so a file written between any two of them still loads.
        auto hasMore = [&in] { return in.rdbuf()->sgetc() != std::char_traits<char>::eof(); };
        auto &p = out.patterns[0];
        IOUtilities::readAndDecode(in, &p.edgeEnabled);
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &p.edgeColor.r);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &p.edgeColor.g);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &p.edgeColor.b);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &p.edgeWidth);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &p.edgeOpacity);
        }
    }

    namespace {
        bool trailerHasMore(std::ifstream &in) {
            return in.rdbuf()->sgetc() != std::char_traits<char>::eof();
        }

        // A file may name a mode this build no longer has; fall back to the ceiling every earlier
        // version rendered under rather than leave the choice unset.
        ShdFogBlurQuality readFogBlurQuality(std::ifstream &in) {
            int32_t quality = 0;
            IOUtilities::readAndDecode(in, &quality);
            return quality == 1 ? ShdFogBlurQuality::APPEARANCE : ShdFogBlurQuality::SPEED;
        }

        void writeChromaticShading(std::ostream &out, const ShaderAttribute &shader) {
            const auto &s = shader.slope;
            IOUtilities::encodeAndWrite(out, s.lumaAmount);
            IOUtilities::encodeAndWrite(out, s.tintResponse);
            IOUtilities::encodeAndWrite(out, s.shadowChroma);
            IOUtilities::encodeAndWrite(out, static_cast<int32_t>(s.tintBlend));
        }

        void readChromaticShading(std::ifstream &in, ShaderAttribute &out) {
            // Guarded one field at a time, so a file written between any two of them still loads.
            auto &s = out.slope;
            IOUtilities::readAndDecode(in, &s.lumaAmount);
            if (trailerHasMore(in)) {
                IOUtilities::readAndDecode(in, &s.tintResponse);
            }
            if (trailerHasMore(in)) {
                IOUtilities::readAndDecode(in, &s.shadowChroma);
            }
            if (trailerHasMore(in)) {
                int32_t tintBlend;
                IOUtilities::readAndDecode(in, &tintBlend);
                // A file may name a mode this build no longer has; fall back to the original composite.
                s.tintBlend = tintBlend == 1 ? ShdSlopeTintBlend::OKLAB : ShdSlopeTintBlend::MULTIPLY;
            }
        }

        void writeFocusBand(std::ostream &out, const ShaderAttribute &shader) {
            const auto &f = shader.fog;
            IOUtilities::encodeAndWrite(out, f.focusAmount);
            IOUtilities::encodeAndWrite(out, f.focusRatio);
            IOUtilities::encodeAndWrite(out, f.focusRange);
            IOUtilities::encodeAndWrite(out, f.focusFalloff);
            IOUtilities::encodeAndWrite(out, f.focusBlur);
        }

        // One whole pattern layer, in the order the shader's own layer block is laid out.
        void writePatternLayer(std::ostream &out, const ShdPatternAttribute &p) {
            IOUtilities::encodeAndWrite(out, p.enabled);
            IOUtilities::encodeAndWrite(out, static_cast<int32_t>(p.type));
            IOUtilities::encodeAndWrite(out, static_cast<int32_t>(p.uvMode));
            IOUtilities::encodeAndWrite(out, static_cast<int32_t>(p.blendMode));
            IOUtilities::encodeAndWrite(out, p.opacity);
            IOUtilities::encodeAndWrite(out, p.scaleU);
            IOUtilities::encodeAndWrite(out, p.scaleV);
            IOUtilities::encodeAndWrite(out, p.scrollU);
            IOUtilities::encodeAndWrite(out, p.scrollV);
            IOUtilities::encodeAndWrite(out, p.paletteFollow);
            IOUtilities::encodeAndWrite(out, p.periodIterations);
            IOUtilities::encodeAndWrite(out, p.sharpness);
            IOUtilities::encodeAndWrite(out, p.color.r);
            IOUtilities::encodeAndWrite(out, p.color.g);
            IOUtilities::encodeAndWrite(out, p.color.b);
            IOUtilities::encodeAndWrite(out, static_cast<int32_t>(p.inkMode));
            IOUtilities::encodeAndWrite(out, p.paletteShift);
            IOUtilities::encodeAndWrite(out, p.edgeEnabled);
            IOUtilities::encodeAndWrite(out, p.edgeColor.r);
            IOUtilities::encodeAndWrite(out, p.edgeColor.g);
            IOUtilities::encodeAndWrite(out, p.edgeColor.b);
            IOUtilities::encodeAndWrite(out, p.edgeWidth);
            IOUtilities::encodeAndWrite(out, p.edgeOpacity);
            IOUtilities::encodeAndWrite(out, p.edgeRelative);
        }

        // Read whole rather than field by field: a layer is only ever written complete, so a file
        // that stops inside one is truncated, and the caller's own guard leaves the rest at default.
        void readPatternLayer(std::ifstream &in, ShdPatternAttribute &p) {
            IOUtilities::readAndDecode(in, &p.enabled);
            int32_t type;
            IOUtilities::readAndDecode(in, &type);
            p.type = type >= 0 && type <= 7 ? static_cast<ShdPatternType>(type) : ShdPatternType::STRIPES;
            int32_t uvMode;
            IOUtilities::readAndDecode(in, &uvMode);
            p.uvMode = uvMode >= 0 && uvMode <= 3 ? static_cast<ShdTextureUVMode>(uvMode)
                                                  : ShdTextureUVMode::CYCLE_BAND;
            int32_t blendMode;
            IOUtilities::readAndDecode(in, &blendMode);
            p.blendMode = static_cast<ShdTextureBlendMode>(blendMode);
            IOUtilities::readAndDecode(in, &p.opacity);
            IOUtilities::readAndDecode(in, &p.scaleU);
            IOUtilities::readAndDecode(in, &p.scaleV);
            IOUtilities::readAndDecode(in, &p.scrollU);
            IOUtilities::readAndDecode(in, &p.scrollV);
            IOUtilities::readAndDecode(in, &p.paletteFollow);
            IOUtilities::readAndDecode(in, &p.periodIterations);
            IOUtilities::readAndDecode(in, &p.sharpness);
            IOUtilities::readAndDecode(in, &p.color.r);
            IOUtilities::readAndDecode(in, &p.color.g);
            IOUtilities::readAndDecode(in, &p.color.b);
            int32_t inkMode;
            IOUtilities::readAndDecode(in, &inkMode);
            p.inkMode = inkMode >= 0 && inkMode <= 1 ? static_cast<ShdPatternInkMode>(inkMode)
                                                     : ShdPatternInkMode::PALETTE_SHIFT;
            IOUtilities::readAndDecode(in, &p.paletteShift);
            IOUtilities::readAndDecode(in, &p.edgeEnabled);
            IOUtilities::readAndDecode(in, &p.edgeColor.r);
            IOUtilities::readAndDecode(in, &p.edgeColor.g);
            IOUtilities::readAndDecode(in, &p.edgeColor.b);
            IOUtilities::readAndDecode(in, &p.edgeWidth);
            IOUtilities::readAndDecode(in, &p.edgeOpacity);
            IOUtilities::readAndDecode(in, &p.edgeRelative);
        }

        void readFocusBand(std::ifstream &in, ShaderAttribute &out) {
            auto &f = out.fog;
            IOUtilities::readAndDecode(in, &f.focusAmount);
            if (trailerHasMore(in)) {
                IOUtilities::readAndDecode(in, &f.focusRatio);
            }
            if (trailerHasMore(in)) {
                IOUtilities::readAndDecode(in, &f.focusRange);
            }
            if (trailerHasMore(in)) {
                IOUtilities::readAndDecode(in, &f.focusFalloff);
            }
            if (trailerHasMore(in)) {
                IOUtilities::readAndDecode(in, &f.focusBlur);
            }
        }
    } // namespace

    void ShaderPresetIO::writeTrailer(std::ostream &out, const ShaderAttribute &shader) {
        IOUtilities::encodeAndWrite(out, TRAILER_MAGIC);
        writeChromaticShading(out, shader);
        writeFocusBand(out, shader);
        IOUtilities::encodeAndWrite(out, shader.patterns[0].edgeRelative);
        for (uint32_t layer = 1; layer < PATTERN_LAYER_COUNT; ++layer) {
            writePatternLayer(out, shader.patterns[layer]);
        }
    }

    void ShaderPresetIO::readTrailer(std::ifstream &in, ShaderAttribute &out) {
        // A read that already failed is a truncated file, and the caller has to keep hearing about
        // it; only what this block itself runs short of is cleared below.
        if (in.fail()) {
            return;
        }
        uint32_t magic = 0;
        IOUtilities::readAndDecode(in, &magic);
        const bool recognized = !in.fail() && magic == TRAILER_MAGIC;
        if (recognized) {
            readChromaticShading(in, out);
            if (trailerHasMore(in)) {
                readFocusBand(in, out);
            }
            if (trailerHasMore(in)) {
                IOUtilities::readAndDecode(in, &out.patterns[0].edgeRelative);
            }
            // Guarded per layer, not once: a file from a build with a shorter stack stops early and
            // leaves the layers it never wrote at their defaults, which is every layer switched off.
            for (uint32_t layer = 1; layer < PATTERN_LAYER_COUNT && trailerHasMore(in); ++layer) {
                readPatternLayer(in, out.patterns[layer]);
            }
        }
        // Fewer bytes here than the block asked for means it is simply not in this file - a marker
        // that is only part of a word, or an older build's leftover field. Nothing is read past this
        // point, so the shortfall is not corruption and must not be reported as it.
        if (in.fail() && !recognized) {
            in.clear();
        }
    }

    void ShaderPresetIO::writeHdr(std::ostream &out, const ShaderAttribute &shader) {
        const auto &h = shader.hdr;
        IOUtilities::encodeAndWrite(out, h.use);
        IOUtilities::encodeAndWrite(out, h.exposure);
        IOUtilities::encodeAndWrite(out, h.headroom);
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(h.method));
    }

    void ShaderPresetIO::readHdr(std::ifstream &in, ShaderAttribute &out) {
        auto hasMore = [&in] { return in.rdbuf()->sgetc() != std::char_traits<char>::eof(); };
        auto &h = out.hdr;
        IOUtilities::readAndDecode(in, &h.use);
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &h.exposure);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &h.headroom);
        }
        if (hasMore()) {
            int32_t method = 0;
            IOUtilities::readAndDecode(in, &method);
            // A file may name a curve this build no longer has; fall back to the straight cut.
            h.method =
                method >= 0 && method <= 7 ? static_cast<ShdToneMapMethod>(method) : ShdToneMapMethod::CLIP;
        }
    }

    void ShaderPresetIO::writeChaosBlur(std::ostream &out, const ShaderAttribute &shader) {
        IOUtilities::encodeAndWrite(out, uint32_t(0x43484231));
        IOUtilities::encodeAndWrite(out, shader.fog.chaosAmount);
        IOUtilities::encodeAndWrite(out, shader.fog.chaosScale);
        IOUtilities::encodeAndWrite(out, shader.fog.chaosThreshold);
        IOUtilities::encodeAndWrite(out, shader.fog.chaosTransition);
        IOUtilities::encodeAndWrite(out, shader.fog.chaosFeather);
        IOUtilities::encodeAndWrite(out, shader.fog.chaosBlur);
        IOUtilities::encodeAndWrite(out, shader.fog.chaosHighlights);
        IOUtilities::encodeAndWrite(out, shader.fog.chaosShade);
    }

    void ShaderPresetIO::readChaosBlur(std::ifstream &in, ShaderAttribute &shader) {
        const ShdFogAttribute defaults{};
        shader.fog.chaosAmount = defaults.chaosAmount;
        shader.fog.chaosScale = defaults.chaosScale;
        shader.fog.chaosThreshold = defaults.chaosThreshold;
        shader.fog.chaosTransition = defaults.chaosTransition;
        shader.fog.chaosFeather = defaults.chaosFeather;
        shader.fog.chaosBlur = defaults.chaosBlur;
        shader.fog.chaosHighlights = defaults.chaosHighlights;
        shader.fog.chaosShade = defaults.chaosShade;
        const auto hasMore = [&in] {
            return !in.fail() && in.rdbuf()->sgetc() != std::char_traits<char>::eof();
        };
        if (!hasMore()) {
            return;
        }
        uint32_t marker = 0;
        IOUtilities::readAndDecode(in, &marker);
        if (in.fail() || marker != 0x43484231) {
            in.setstate(std::ios::failbit);
            return;
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &shader.fog.chaosAmount);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &shader.fog.chaosScale);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &shader.fog.chaosThreshold);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &shader.fog.chaosTransition);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &shader.fog.chaosFeather);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &shader.fog.chaosBlur);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &shader.fog.chaosHighlights);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &shader.fog.chaosShade);
        }
    }

    void ShaderPresetIO::writeSurfaceReplacement(std::ostream &out, const ShaderAttribute &shader) {
        IOUtilities::encodeAndWrite(out, uint32_t(0x53525031));
        IOUtilities::encodeAndWrite(out, uint32_t(shader.slope.replaceSurfaceStyle));
    }

    void ShaderPresetIO::readSurfaceReplacement(std::ifstream &in, ShaderAttribute &shader) {
        shader.slope.replaceSurfaceStyle = false;
        if (in.fail() || in.rdbuf()->sgetc() == std::char_traits<char>::eof()) {
            return;
        }
        uint32_t marker = 0, enabled = 0;
        IOUtilities::readAndDecode(in, &marker);
        if (in.fail() || marker != 0x53525031) {
            in.setstate(std::ios::failbit);
            return;
        }
        if (in.rdbuf()->sgetc() == std::char_traits<char>::eof()) {
            return;
        }
        IOUtilities::readAndDecode(in, &enabled);
        if (in.fail() || enabled > 1) {
            in.setstate(std::ios::failbit);
            return;
        }
        shader.slope.replaceSurfaceStyle = enabled != 0;
    }

    void ShaderPresetIO::writeBandDecorations(std::ostream &out, const ShaderAttribute &shader) {
        IOUtilities::encodeAndWrite(out, uint32_t(0x42445331));
        IOUtilities::encodeAndWrite(out, shader.palette.bandSpineAmount);
        IOUtilities::encodeAndWrite(out, shader.palette.bandSpineLength);
        IOUtilities::encodeAndWrite(out, shader.palette.bandSpineDensity);
        IOUtilities::encodeAndWrite(out, shader.palette.bandBranchAmount);
        IOUtilities::encodeAndWrite(out, shader.palette.bandOrnamentAmount);
        IOUtilities::encodeAndWrite(out, shader.palette.bandOrnamentSize);
        IOUtilities::encodeAndWrite(out, shader.palette.bandOrnamentDensity);
        IOUtilities::encodeAndWrite(out, shader.palette.bandOrnamentInset);
    }

    void ShaderPresetIO::readBandDecorations(std::ifstream &in, ShaderAttribute &shader) {
        shader.palette.bandSpineAmount = 0.0f;
        shader.palette.bandSpineLength = 1.0f;
        shader.palette.bandSpineDensity = 1.0f;
        shader.palette.bandBranchAmount = 1.0f;
        shader.palette.bandOrnamentAmount = 0.0f;
        shader.palette.bandOrnamentSize = 1.0f;
        shader.palette.bandOrnamentDensity = 1.0f;
        shader.palette.bandOrnamentInset = 0.032f;
        const auto hasMore = [&in] {
            return !in.fail() && in.rdbuf()->sgetc() != std::char_traits<char>::eof();
        };
        if (!hasMore()) {
            return;
        }
        uint32_t marker = 0;
        IOUtilities::readAndDecode(in, &marker);
        if (in.fail() || marker != 0x42445331) {
            in.setstate(std::ios::failbit);
            return;
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &shader.palette.bandSpineAmount);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &shader.palette.bandSpineLength);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &shader.palette.bandSpineDensity);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &shader.palette.bandBranchAmount);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &shader.palette.bandOrnamentAmount);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &shader.palette.bandOrnamentSize);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &shader.palette.bandOrnamentDensity);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &shader.palette.bandOrnamentInset);
        }
    }

    void ShaderPresetIO::writeBandLine(std::ostream &out, const ShaderAttribute &shader) {
        const auto &p = shader.palette;
        IOUtilities::encodeAndWrite(out, p.bandLineEnabled);
        IOUtilities::encodeAndWrite(out, p.bandLineCount);
        IOUtilities::encodeAndWrite(out, p.bandLineWidth);
        IOUtilities::encodeAndWrite(out, p.bandLineOpacity);
        IOUtilities::encodeAndWrite(out, p.bandLineColor.r);
        IOUtilities::encodeAndWrite(out, p.bandLineColor.g);
        IOUtilities::encodeAndWrite(out, p.bandLineColor.b);
        IOUtilities::encodeAndWrite(out, p.bandLineSoftness);
    }

    void ShaderPresetIO::readBandLine(std::ifstream &in, ShaderAttribute &out) {
        // Guarded one field at a time, so a file written between any two of them still loads.
        auto hasMore = [&in] { return in.rdbuf()->sgetc() != std::char_traits<char>::eof(); };
        auto &p = out.palette;
        IOUtilities::readAndDecode(in, &p.bandLineEnabled);
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &p.bandLineCount);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &p.bandLineWidth);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &p.bandLineOpacity);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &p.bandLineColor.r);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &p.bandLineColor.g);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &p.bandLineColor.b);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &p.bandLineSoftness);
        }
        p.bandLineColor.a = 1.0f;
    }

    void ShaderPresetIO::writeGloss(std::ostream &out, const ShaderAttribute &shader) {
        const auto &s = shader.slope;
        IOUtilities::encodeAndWrite(out, GLOSS_MAGIC);
        IOUtilities::encodeAndWrite(out, s.glossIntensity);
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(s.glossSource));
        IOUtilities::encodeAndWrite(out, s.glossBands);
        IOUtilities::encodeAndWrite(out, s.glossSharpness);
        IOUtilities::encodeAndWrite(out, s.glossPhase);
        IOUtilities::encodeAndWrite(out, s.glossColor.r);
        IOUtilities::encodeAndWrite(out, s.glossColor.g);
        IOUtilities::encodeAndWrite(out, s.glossColor.b);
    }

    void ShaderPresetIO::readGloss(std::ifstream &in, ShaderAttribute &out) {
        // A read that already failed is a truncated file, and the caller has to keep hearing about
        // it; only what this block itself runs short of is cleared below.
        if (in.fail()) {
            return;
        }
        uint32_t magic = 0;
        IOUtilities::readAndDecode(in, &magic);
        const bool recognized = !in.fail() && magic == GLOSS_MAGIC;
        if (recognized) {
            // Guarded one field at a time, so a file written between any two of them still loads.
            auto hasMore = [&in] { return in.rdbuf()->sgetc() != std::char_traits<char>::eof(); };
            auto &s = out.slope;
            IOUtilities::readAndDecode(in, &s.glossIntensity);
            if (hasMore()) {
                int32_t source = 0;
                IOUtilities::readAndDecode(in, &source);
                // A file may name a coordinate this build no longer has; fall back to the shading.
                s.glossSource = source >= 0 && source <= 3 ? static_cast<ShdSlopeGlossSource>(source)
                                                           : ShdSlopeGlossSource::SHADING;
            }
            if (hasMore()) {
                IOUtilities::readAndDecode(in, &s.glossBands);
            }
            if (hasMore()) {
                IOUtilities::readAndDecode(in, &s.glossSharpness);
            }
            if (hasMore()) {
                IOUtilities::readAndDecode(in, &s.glossPhase);
            }
            if (hasMore()) {
                IOUtilities::readAndDecode(in, &s.glossColor.r);
            }
            if (hasMore()) {
                IOUtilities::readAndDecode(in, &s.glossColor.g);
            }
            if (hasMore()) {
                IOUtilities::readAndDecode(in, &s.glossColor.b);
            }
            s.glossColor.a = 1.0f;
        }
        // Fewer bytes here than the block asked for means it is simply not in this file - a marker
        // that is only part of a word, or an older build's leftover field. Nothing is read past this
        // point, so the shortfall is not corruption and must not be reported as it.
        if (in.fail() && !recognized) {
            in.clear();
        }
    }

    void ShaderPresetIO::writePaletteColoring(std::ostream &out, const ShaderAttribute &shader) {
        IOUtilities::encodeAndWrite(out, PALETTE_COLORING_MAGIC);
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(shader.palette.iterationColoring));
    }

    void ShaderPresetIO::readPaletteColoring(std::ifstream &in, ShaderAttribute &out) {
        // A read that already failed is a truncated file, and the caller has to keep hearing about it.
        if (in.fail()) {
            return;
        }
        uint32_t magic = 0;
        IOUtilities::readAndDecode(in, &magic);
        const bool recognized = !in.fail() && magic == PALETTE_COLORING_MAGIC;
        if (recognized) {
            int32_t mode = 0;
            IOUtilities::readAndDecode(in, &mode);
            // A file may name a curve this build no longer has; fall back to the straight count.
            out.palette.iterationColoring = mode >= 0 && mode <= 6
                                                ? static_cast<ShdPalIterationColoringMode>(mode)
                                                : ShdPalIterationColoringMode::LINEAR;
        }
        // Short of what the block asked for means it is simply not in this file, which is not corruption.
        if (in.fail() && !recognized) {
            in.clear();
        }
    }

    void ShaderPresetIO::writeGlossRelief(std::ostream &out, const ShaderAttribute &shader) {
        IOUtilities::encodeAndWrite(out, GLOSS_RELIEF_MAGIC);
        IOUtilities::encodeAndWrite(out, shader.slope.glossRelief);
    }

    void ShaderPresetIO::readGlossRelief(std::ifstream &in, ShaderAttribute &out) {
        // A read that already failed is a truncated file, and the caller has to keep hearing about it.
        if (in.fail()) {
            return;
        }
        uint32_t magic = 0;
        IOUtilities::readAndDecode(in, &magic);
        const bool recognized = !in.fail() && magic == GLOSS_RELIEF_MAGIC;
        if (recognized) {
            float relief = 0.0f;
            IOUtilities::readAndDecode(in, &relief);
            if (!in.fail()) {
                out.slope.glossRelief = relief;
            }
        }
        // Short of what the block asked for means it is simply not in this file, which is not corruption.
        if (in.fail() && !recognized) {
            in.clear();
        }
    }

    void ShaderPresetIO::writeStudioLighting(std::ostream &out, const ShaderAttribute &shader) {
        IOUtilities::encodeAndWrite(out, STUDIO_LIGHTING_MAGIC);
        const auto &s = shader.slope;
        IOUtilities::encodeAndWrite(out, s.studioEnvironmentRotation);
        IOUtilities::encodeAndWrite(out, s.studioEnvironmentFollow);
    }

    void ShaderPresetIO::readStudioLighting(std::ifstream &in, ShaderAttribute &shader, bool legacyLayout) {
        auto &s = shader.slope;
        const ShdSlopeAttribute defaults{};
        s.studioEnvironmentRotation = defaults.studioEnvironmentRotation;
        s.studioEnvironmentFollow = defaults.studioEnvironmentFollow;
        const auto more = [&] { return !in.fail() && in.rdbuf()->sgetc() != std::char_traits<char>::eof(); };
        if (!more()) {
            return;
        }
        uint32_t magic = 0;
        IOUtilities::readAndDecode(in, &magic);
        if (in.fail() || magic != STUDIO_LIGHTING_MAGIC) {
            in.setstate(std::ios::failbit);
            return;
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.studioEnvironmentRotation);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.studioEnvironmentFollow);
        }
        readRetiredFloats(in, legacyLayout, 32);
    }

    void ShaderPresetIO::writeStudio(std::ostream &out, const ShaderAttribute &shader) {
        IOUtilities::encodeAndWrite(out, STUDIO_MAGIC);
        const auto &s = shader.slope.studio;
        IOUtilities::encodeAndWrite(out, uint32_t(s.use ? 1 : 0));
        IOUtilities::encodeAndWrite(out, s.roughness);
        IOUtilities::encodeAndWrite(out, s.metalness);
        IOUtilities::encodeAndWrite(out, s.ior);
        IOUtilities::encodeAndWrite(out, s.directIntensity);
        IOUtilities::encodeAndWrite(out, s.environmentIntensity);
        IOUtilities::encodeAndWrite(out, s.clearcoat);
        IOUtilities::encodeAndWrite(out, s.clearcoatRoughness);
    }

    void ShaderPresetIO::readStudio(std::ifstream &in, ShaderAttribute &out, bool legacyLayout) {
        out.slope.studio = {};
        if (in.fail() || in.rdbuf()->sgetc() == std::char_traits<char>::eof()) {
            return;
        }
        uint32_t magic = 0;
        IOUtilities::readAndDecode(in, &magic);
        if (in.fail() || magic != STUDIO_MAGIC) {
            return;
        }
        auto &s = out.slope.studio;
        auto hasMore = [&in] { return !in.fail() && in.rdbuf()->sgetc() != std::char_traits<char>::eof(); };
        uint32_t model = 0;
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &model);
        }
        if (model > 1) {
            in.setstate(std::ios::failbit);
            return;
        }
        s.use = model == 1;
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.roughness);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.metalness);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.ior);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.directIntensity);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.environmentIntensity);
        }
        if (hasMore()) {
            if (legacyLayout) { float discarded; IOUtilities::readAndDecode(in, &discarded); }
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.clearcoat);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.clearcoatRoughness);
        }
    }

    void ShaderPresetIO::writeEffectSync(std::ostream &out, const ShaderAttribute &shader) {
        for (const auto &e : shader.effects) {
            IOUtilities::encodeAndWrite(out, uint32_t(e.syncColorAnimation));
        }
    }

    void ShaderPresetIO::readEffectSync(std::ifstream &in, ShaderAttribute &shader) {
        for (auto &e : shader.effects) {
            e.syncColorAnimation = false;
            if (in.fail() || in.rdbuf()->sgetc() == std::char_traits<char>::eof()) {
                continue;
            }
            uint32_t enabled = 0;
            IOUtilities::readAndDecode(in, &enabled);
            if (in.fail() || enabled > 1) {
                in.setstate(std::ios::failbit);
                return;
            }
            e.syncColorAnimation = enabled != 0;
        }
    }

    void ShaderPresetIO::writeLayerOrder(std::ostream &out, const ShaderAttribute &shader) {
        IOUtilities::encodeAndWrite(out, uint32_t(0x4C415931));
        IOUtilities::encodeAndWrite(out, uint32_t(shader.layerOrder.enabled));
        IOUtilities::encodeAndWrite(out, ShdLayerOrder::COUNT);
        for (const auto layer : shader.layerOrder.layers) {
            IOUtilities::encodeAndWrite(out, uint32_t(layer));
        }
        IOUtilities::encodeAndWrite(out, uint32_t(0x56495331));
        IOUtilities::encodeAndWrite(out, shader.layerOrder.hiddenMask);
    }

    void ShaderPresetIO::readLayerOrder(std::ifstream &in, ShaderAttribute &shader) {
        shader.layerOrder = {};
        if (in.fail() || in.rdbuf()->sgetc() == std::char_traits<char>::eof()) {
            return;
        }
        uint32_t marker = 0, enabled = 0, count = 0;
        IOUtilities::readAndDecode(in, &marker);
        IOUtilities::readAndDecode(in, &enabled);
        IOUtilities::readAndDecode(in, &count);
        if (in.fail() || marker != 0x4C415931 || enabled > 1 || count != ShdLayerOrder::COUNT) {
            in.setstate(std::ios::failbit);
            return;
        }
        ShdLayerOrder order;
        order.enabled = enabled != 0;
        for (auto &layer : order.layers) {
            uint32_t id = 0;
            IOUtilities::readAndDecode(in, &id);
            layer = ShdLayer(id);
        }
        if (!in.fail() && in.rdbuf()->sgetc() != std::char_traits<char>::eof()) {
            uint32_t visibilityMarker = 0;
            IOUtilities::readAndDecode(in, &visibilityMarker);
            IOUtilities::readAndDecode(in, &order.hiddenMask);
            if (visibilityMarker != 0x56495331) {
                in.setstate(std::ios::failbit);
            }
        }
        if (in.fail() || !order.valid()) {
            in.setstate(std::ios::failbit);
            return;
        }
        shader.layerOrder = order;
    }

    void ShaderPresetIO::writeChrome(std::ostream &out, const ShaderAttribute &shader) {
        const auto &s = shader.slope;
        IOUtilities::encodeAndWrite(out, uint32_t(s.surfaceStyle));
        IOUtilities::encodeAndWrite(out, s.chromeStrength);
        IOUtilities::encodeAndWrite(out, s.filmStrength);
        IOUtilities::encodeAndWrite(out, s.reflectionDetail);
        IOUtilities::encodeAndWrite(out, s.reflectionContrast);
        IOUtilities::encodeAndWrite(out, s.reflectionBrightness);
        IOUtilities::encodeAndWrite(out, s.reflectionCurve);
        IOUtilities::encodeAndWrite(out, s.surfacePhase);
        IOUtilities::encodeAndWrite(out, s.prismWidth);
        IOUtilities::encodeAndWrite(out, s.prismSpread);
        IOUtilities::encodeAndWrite(out, s.filmHue);
        IOUtilities::encodeAndWrite(out, s.sparkleStrength);
        IOUtilities::encodeAndWrite(out, s.backgroundBrightness);
        IOUtilities::encodeAndWrite(out, s.shadowCrush);
        IOUtilities::encodeAndWrite(out, s.flameStrength);
        IOUtilities::encodeAndWrite(out, s.phonkRed);
        IOUtilities::encodeAndWrite(out, s.vhsNoise);
        IOUtilities::encodeAndWrite(out, s.chromaticShift);
        IOUtilities::encodeAndWrite(out, s.pixelMix);
        IOUtilities::encodeAndWrite(out, s.pixelSize);
        IOUtilities::encodeAndWrite(out, s.filmGrain);
        IOUtilities::encodeAndWrite(out, s.frostStrength);
        IOUtilities::encodeAndWrite(out, s.frostThreshold);
        IOUtilities::encodeAndWrite(out, s.grungeScale);
        IOUtilities::encodeAndWrite(out, s.paletteColorMix);
        IOUtilities::encodeAndWrite(out, s.styleColorMix);
        IOUtilities::encodeAndWrite(out, s.styleHighlightMix);
        IOUtilities::encodeAndWrite(out, s.styleBackgroundMix);
        IOUtilities::encodeAndWrite(out, s.styleRimStrength);
        IOUtilities::encodeAndWrite(out, s.styleRimWidth);
        IOUtilities::encodeAndWrite(out, s.styleInkPreserve);
        IOUtilities::encodeAndWrite(out, s.styleMonochrome);
        IOUtilities::encodeAndWrite(out, s.styleGlintSize);
        IOUtilities::encodeAndWrite(out, s.styleDetailLight);
        IOUtilities::encodeAndWrite(out, s.styleDetailSuppress);
        IOUtilities::encodeAndWrite(out, s.styleDetailThreshold);
        IOUtilities::encodeAndWrite(out, s.styleDamage);
        IOUtilities::encodeAndWrite(out, static_cast<uint32_t>(s.surfaceBlend));
        IOUtilities::encodeAndWrite(out, s.styleColor.r);
        IOUtilities::encodeAndWrite(out, s.styleColor.g);
        IOUtilities::encodeAndWrite(out, s.styleColor.b);
        IOUtilities::encodeAndWrite(out, s.styleHighlightColor.r);
        IOUtilities::encodeAndWrite(out, s.styleHighlightColor.g);
        IOUtilities::encodeAndWrite(out, s.styleHighlightColor.b);
        IOUtilities::encodeAndWrite(out, s.styleBackgroundColor.r);
        IOUtilities::encodeAndWrite(out, s.styleBackgroundColor.g);
        IOUtilities::encodeAndWrite(out, s.styleBackgroundColor.b);
        IOUtilities::encodeAndWrite(out, s.styleRimColor.r);
        IOUtilities::encodeAndWrite(out, s.styleRimColor.g);
        IOUtilities::encodeAndWrite(out, s.styleRimColor.b);
        IOUtilities::encodeAndWrite(out, s.seaGlow);
        IOUtilities::encodeAndWrite(out, s.seaThreshold);
        IOUtilities::encodeAndWrite(out, s.seaBody);
        IOUtilities::encodeAndWrite(out, s.seaParticles);
        IOUtilities::encodeAndWrite(out, s.seaParticleSize);
        IOUtilities::encodeAndWrite(out, s.seaBalance);
        IOUtilities::encodeAndWrite(out, s.ukiyoColors);
        IOUtilities::encodeAndWrite(out, s.ukiyoFoam);
        IOUtilities::encodeAndWrite(out, s.ukiyoGrain);
        IOUtilities::encodeAndWrite(out, s.ukiyoFlatness);
        IOUtilities::encodeAndWrite(out, s.ukiyoBalance);
        IOUtilities::encodeAndWrite(out, s.seaBodyColor.r);
        IOUtilities::encodeAndWrite(out, s.seaBodyColor.g);
        IOUtilities::encodeAndWrite(out, s.seaBodyColor.b);
        IOUtilities::encodeAndWrite(out, s.seaGlowColor.r);
        IOUtilities::encodeAndWrite(out, s.seaGlowColor.g);
        IOUtilities::encodeAndWrite(out, s.seaGlowColor.b);
        IOUtilities::encodeAndWrite(out, s.seaAccentColor.r);
        IOUtilities::encodeAndWrite(out, s.seaAccentColor.g);
        IOUtilities::encodeAndWrite(out, s.seaAccentColor.b);
        IOUtilities::encodeAndWrite(out, s.printInk.r);
        IOUtilities::encodeAndWrite(out, s.printInk.g);
        IOUtilities::encodeAndWrite(out, s.printInk.b);
        IOUtilities::encodeAndWrite(out, s.printIndigo.r);
        IOUtilities::encodeAndWrite(out, s.printIndigo.g);
        IOUtilities::encodeAndWrite(out, s.printIndigo.b);
        IOUtilities::encodeAndWrite(out, s.printAsagi.r);
        IOUtilities::encodeAndWrite(out, s.printAsagi.g);
        IOUtilities::encodeAndWrite(out, s.printAsagi.b);
        IOUtilities::encodeAndWrite(out, s.printBlue.r);
        IOUtilities::encodeAndWrite(out, s.printBlue.g);
        IOUtilities::encodeAndWrite(out, s.printBlue.b);
        IOUtilities::encodeAndWrite(out, s.printFoam.r);
        IOUtilities::encodeAndWrite(out, s.printFoam.g);
        IOUtilities::encodeAndWrite(out, s.printFoam.b);
        IOUtilities::encodeAndWrite(out, s.printPaper.r);
        IOUtilities::encodeAndWrite(out, s.printPaper.g);
        IOUtilities::encodeAndWrite(out, s.printPaper.b);
        IOUtilities::encodeAndWrite(out, s.layerMetal);
        IOUtilities::encodeAndWrite(out, s.layerSigil);
        IOUtilities::encodeAndWrite(out, s.layerPhonk);
        IOUtilities::encodeAndWrite(out, s.layerFrost);
        IOUtilities::encodeAndWrite(out, s.layerSea);
        IOUtilities::encodeAndWrite(out, s.layerPrint);
        IOUtilities::encodeAndWrite(out, s.layerQuantize);
        IOUtilities::encodeAndWrite(out, s.layerVhs);
        IOUtilities::encodeAndWrite(out, s.layerMono);
        IOUtilities::encodeAndWrite(out, s.layerCommon);
    }

    void ShaderPresetIO::readChrome(std::ifstream &in, ShaderAttribute &shader, bool legacyLayout) {
        auto &s = shader.slope;
        s.surfaceStyle = ShdSurfaceStyle::ORIGINAL;
        s.chromeStrength = 1;
        s.filmStrength = 0.65f;
        auto more = [&] { return !in.fail() && in.rdbuf()->sgetc() != std::char_traits<char>::eof(); };
        uint32_t mode = 0;
        if (more()) {
            IOUtilities::readAndDecode(in, &mode);
        }
        if (mode > 8) {
            in.setstate(std::ios::failbit);
        }
        // Retired experimental style IDs load as Original without shifting later fields.
        s.surfaceStyle =
            mode == 7 || mode == 8 ? ShdSurfaceStyle::ORIGINAL : static_cast<ShdSurfaceStyle>(mode);
        if (more()) {
            IOUtilities::readAndDecode(in, &s.chromeStrength);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.filmStrength);
        }
        readRetiredFloats(in, legacyLayout, 1);
        const ShdSlopeAttribute defaults{};
        s.reflectionDetail = defaults.reflectionDetail;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.reflectionDetail);
        }
        s.reflectionContrast = defaults.reflectionContrast;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.reflectionContrast);
        }
        s.reflectionBrightness = defaults.reflectionBrightness;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.reflectionBrightness);
        }
        s.reflectionCurve = defaults.reflectionCurve;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.reflectionCurve);
        }
        s.surfacePhase = defaults.surfacePhase;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.surfacePhase);
        }
        s.prismWidth = defaults.prismWidth;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.prismWidth);
        }
        s.prismSpread = defaults.prismSpread;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.prismSpread);
        }
        s.filmHue = defaults.filmHue;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.filmHue);
        }
        s.sparkleStrength = defaults.sparkleStrength;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.sparkleStrength);
        }
        readRetiredFloats(in, legacyLayout, 6);
        s.backgroundBrightness = defaults.backgroundBrightness;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.backgroundBrightness);
        }
        s.shadowCrush = defaults.shadowCrush;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.shadowCrush);
        }
        s.flameStrength = defaults.flameStrength;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.flameStrength);
        }
        s.phonkRed = defaults.phonkRed;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.phonkRed);
        }
        s.vhsNoise = defaults.vhsNoise;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.vhsNoise);
        }
        s.chromaticShift = defaults.chromaticShift;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.chromaticShift);
        }
        s.pixelMix = defaults.pixelMix;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.pixelMix);
        }
        s.pixelSize = defaults.pixelSize;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.pixelSize);
        }
        s.filmGrain = defaults.filmGrain;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.filmGrain);
        }
        s.frostStrength = defaults.frostStrength;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.frostStrength);
        }
        readRetiredFloats(in, legacyLayout, 1);
        s.frostThreshold = defaults.frostThreshold;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.frostThreshold);
        }
        s.grungeScale = defaults.grungeScale;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.grungeScale);
        }
        s.paletteColorMix = defaults.paletteColorMix;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.paletteColorMix);
        }
        s.styleColorMix = defaults.styleColorMix;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.styleColorMix);
        }
        s.styleHighlightMix = defaults.styleHighlightMix;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.styleHighlightMix);
        }
        readRetiredFloats(in, legacyLayout, 1);
        s.styleBackgroundMix = defaults.styleBackgroundMix;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.styleBackgroundMix);
        }
        s.styleRimStrength = defaults.styleRimStrength;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.styleRimStrength);
        }
        s.styleRimWidth = defaults.styleRimWidth;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.styleRimWidth);
        }
        s.styleInkPreserve = defaults.styleInkPreserve;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.styleInkPreserve);
        }
        s.styleMonochrome = defaults.styleMonochrome;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.styleMonochrome);
        }
        s.styleGlintSize = defaults.styleGlintSize;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.styleGlintSize);
        }
        readRetiredFloats(in, legacyLayout, 3);
        s.styleDetailLight = defaults.styleDetailLight;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.styleDetailLight);
        }
        s.styleDetailSuppress = defaults.styleDetailSuppress;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.styleDetailSuppress);
        }
        s.styleDetailThreshold = defaults.styleDetailThreshold;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.styleDetailThreshold);
        }
        s.styleDamage = defaults.styleDamage;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.styleDamage);
        }
        uint32_t blend = 0;
        if (more()) {
            IOUtilities::readAndDecode(in, &blend);
        }
        if (blend > 3) {
            in.setstate(std::ios::failbit);
        }
        s.surfaceBlend = static_cast<ShdSurfaceBlend>(blend);
        s.styleColor = defaults.styleColor;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.styleColor.r);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.styleColor.g);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.styleColor.b);
        }
        s.styleHighlightColor = defaults.styleHighlightColor;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.styleHighlightColor.r);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.styleHighlightColor.g);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.styleHighlightColor.b);
        }
        readRetiredFloats(in, legacyLayout, 3);
        s.styleBackgroundColor = defaults.styleBackgroundColor;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.styleBackgroundColor.r);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.styleBackgroundColor.g);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.styleBackgroundColor.b);
        }
        s.styleRimColor = defaults.styleRimColor;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.styleRimColor.r);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.styleRimColor.g);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.styleRimColor.b);
        }
        s.seaGlow = defaults.seaGlow;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.seaGlow);
        }
        s.seaThreshold = defaults.seaThreshold;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.seaThreshold);
        }
        s.seaBody = defaults.seaBody;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.seaBody);
        }
        s.seaParticles = defaults.seaParticles;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.seaParticles);
        }
        s.seaParticleSize = defaults.seaParticleSize;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.seaParticleSize);
        }
        s.seaBalance = defaults.seaBalance;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.seaBalance);
        }
        s.ukiyoColors = defaults.ukiyoColors;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.ukiyoColors);
        }
        readRetiredFloats(in, legacyLayout, 1);
        s.ukiyoFoam = defaults.ukiyoFoam;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.ukiyoFoam);
        }
        s.ukiyoGrain = defaults.ukiyoGrain;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.ukiyoGrain);
        }
        s.ukiyoFlatness = defaults.ukiyoFlatness;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.ukiyoFlatness);
        }
        s.ukiyoBalance = defaults.ukiyoBalance;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.ukiyoBalance);
        }
        s.seaBodyColor = defaults.seaBodyColor;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.seaBodyColor.r);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.seaBodyColor.g);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.seaBodyColor.b);
        }
        s.seaGlowColor = defaults.seaGlowColor;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.seaGlowColor.r);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.seaGlowColor.g);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.seaGlowColor.b);
        }
        s.seaAccentColor = defaults.seaAccentColor;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.seaAccentColor.r);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.seaAccentColor.g);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.seaAccentColor.b);
        }
        s.printInk = defaults.printInk;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.printInk.r);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.printInk.g);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.printInk.b);
        }
        s.printIndigo = defaults.printIndigo;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.printIndigo.r);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.printIndigo.g);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.printIndigo.b);
        }
        s.printAsagi = defaults.printAsagi;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.printAsagi.r);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.printAsagi.g);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.printAsagi.b);
        }
        s.printBlue = defaults.printBlue;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.printBlue.r);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.printBlue.g);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.printBlue.b);
        }
        s.printFoam = defaults.printFoam;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.printFoam.r);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.printFoam.g);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.printFoam.b);
        }
        s.printPaper = defaults.printPaper;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.printPaper.r);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.printPaper.g);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &s.printPaper.b);
        }
        s.layerMetal = defaults.layerMetal;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.layerMetal);
        }
        s.layerSigil = defaults.layerSigil;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.layerSigil);
        }
        s.layerPhonk = defaults.layerPhonk;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.layerPhonk);
        }
        s.layerFrost = defaults.layerFrost;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.layerFrost);
        }
        s.layerSea = defaults.layerSea;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.layerSea);
        }
        s.layerPrint = defaults.layerPrint;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.layerPrint);
        }
        s.layerQuantize = defaults.layerQuantize;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.layerQuantize);
        }
        s.layerVhs = defaults.layerVhs;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.layerVhs);
        }
        s.layerMono = defaults.layerMono;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.layerMono);
        }
        s.layerCommon = defaults.layerCommon;
        if (more()) {
            IOUtilities::readAndDecode(in, &s.layerCommon);
        }
    }

    void ShaderPresetIO::writeGroove(std::ostream &out, const ShaderAttribute &shader) {
        const auto &p = shader.palette;
        IOUtilities::encodeAndWrite(out, uint32_t(p.bandLineGroove));
        IOUtilities::encodeAndWrite(out, p.grooveDepth);
        IOUtilities::encodeAndWrite(out, p.grooveWidth);
        IOUtilities::encodeAndWrite(out, uint32_t(p.grooveAuto));
    }

    void ShaderPresetIO::readGroove(std::ifstream &in, ShaderAttribute &shader) {
        auto &p = shader.palette;
        p.bandLineGroove = false;
        p.grooveDepth = 1.5f;
        p.grooveWidth = 0.05f;
        p.grooveAuto = true;
        auto more = [&] { return !in.fail() && in.rdbuf()->sgetc() != std::char_traits<char>::eof(); };
        uint32_t enabled = 0, automatic = 1;
        if (more()) {
            IOUtilities::readAndDecode(in, &enabled);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &p.grooveDepth);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &p.grooveWidth);
        }
        if (more()) {
            IOUtilities::readAndDecode(in, &automatic);
        }
        if (enabled > 1 || automatic > 1) {
            in.setstate(std::ios::failbit);
        }
        p.bandLineGroove = enabled != 0;
        p.grooveAuto = automatic != 0;
    }

    void ShaderPresetIO::writeEffects(std::ostream &out, const ShaderAttribute &shader) {
        IOUtilities::encodeAndWrite(out, uint32_t(0x45465831));
        for (const auto &e : shader.effects) {
            IOUtilities::encodeAndWrite(out, uint32_t(e.enabled));
            IOUtilities::encodeAndWrite(out, uint32_t(e.type));
            IOUtilities::encodeAndWrite(out, uint32_t(e.blend));
            IOUtilities::encodeAndWrite(out, uint32_t(e.mask));
            IOUtilities::encodeAndWrite(out, e.opacity);
            IOUtilities::encodeAndWrite(out, e.scale);
            IOUtilities::encodeAndWrite(out, e.speed);
            IOUtilities::encodeAndWrite(out, e.evolution);
            IOUtilities::encodeAndWrite(out, e.density);
            IOUtilities::encodeAndWrite(out, e.length);
            IOUtilities::encodeAndWrite(out, e.width);
            IOUtilities::encodeAndWrite(out, e.direction);
            IOUtilities::encodeAndWrite(out, e.distortion);
            IOUtilities::encodeAndWrite(out, e.glow);
            IOUtilities::encodeAndWrite(out, e.seed);
            IOUtilities::encodeAndWrite(out, e.period);
            for (int j = 0; j < 4; ++j) {
                IOUtilities::encodeAndWrite(out, e.color[j]);
            }
            for (int j = 0; j < 4; ++j) {
                IOUtilities::encodeAndWrite(out, e.secondary[j]);
            }
        }
        IOUtilities::encodeAndWrite(out, uint32_t(0x45465832));
        for (const auto &e : shader.effects) {
            IOUtilities::encodeAndWrite(out, uint32_t(e.rainShape));
            IOUtilities::encodeAndWrite(out, e.dropSize);
        }
    }

    void ShaderPresetIO::readEffects(std::ifstream &in, ShaderAttribute &shader) {
        shader.effects = {};
        auto more = [&] { return !in.fail() && in.rdbuf()->sgetc() != std::char_traits<char>::eof(); };
        if (!more()) {
            return;
        }
        uint32_t magic = 0;
        IOUtilities::readAndDecode(in, &magic);
        if (in.fail() || magic != 0x45465831) {
            in.setstate(std::ios::failbit);
            return;
        }
        auto read = [&](auto &v) {
            if (more()) {
                IOUtilities::readAndDecode(in, &v);
            }
        };
        for (auto &e : shader.effects) {
            e.rainShape = ShdRainShape::STREAKS;
            uint32_t enabled = 0, type = 0, blend = 2, mask = 0;
            read(enabled);
            read(type);
            read(blend);
            read(mask);
            if (enabled > 1 || type > 7 || blend > 3 || mask > 4) {
                in.setstate(std::ios::failbit);
                return;
            }
            e.enabled = enabled != 0;
            e.type = static_cast<ShdEffectType>(type);
            e.blend = static_cast<ShdEffectBlend>(blend);
            e.mask = static_cast<ShdEffectMask>(mask);
            read(e.opacity);
            read(e.scale);
            read(e.speed);
            read(e.evolution);
            read(e.density);
            read(e.length);
            read(e.width);
            read(e.direction);
            read(e.distortion);
            read(e.glow);
            read(e.seed);
            read(e.period);
            for (int j = 0; j < 4; ++j) {
                read(e.color[j]);
            }
            for (int j = 0; j < 4; ++j) {
                read(e.secondary[j]);
            }
        }
        if (!more()) {
            return;
        }
        IOUtilities::readAndDecode(in, &magic);
        if (in.fail() || magic != 0x45465832) {
            in.setstate(std::ios::failbit);
            return;
        }
        for (auto &e : shader.effects) {
            uint32_t shape = 0;
            read(shape);
            if (shape > 1) {
                in.setstate(std::ios::failbit);
                return;
            }
            e.rainShape = static_cast<ShdRainShape>(shape);
            read(e.dropSize);
        }
    }

    void ShaderPresetIO::writeSurface(std::ostream &out, const ShaderAttribute &shader) {
        IOUtilities::encodeAndWrite(out, uint32_t(0x53524631));
        const auto &s = shader.slope;
        IOUtilities::encodeAndWrite(out, s.iridescence);
        IOUtilities::encodeAndWrite(out, s.filmThickness);
        IOUtilities::encodeAndWrite(out, s.specularAA);
        IOUtilities::encodeAndWrite(out, uint32_t(s.lustreRelief));
        IOUtilities::encodeAndWrite(out, s.reliefDepth);
        IOUtilities::encodeAndWrite(out, s.normalSmooth);
        IOUtilities::encodeAndWrite(out, s.aoRadius);
        IOUtilities::encodeAndWrite(out, s.reliefWaves);
        IOUtilities::encodeAndWrite(out, s.waveFrequency);
        IOUtilities::encodeAndWrite(out, uint32_t(s.invertRelief));
        IOUtilities::encodeAndWrite(out, s.boundaryGuard);
        IOUtilities::encodeAndWrite(out, uint32_t(shader.palette.stops.size()));
        for (const auto &stop : shader.palette.stops) {
            IOUtilities::encodeAndWrite(out, stop.position);
            for (int i = 0; i < 4; ++i) {
                IOUtilities::encodeAndWrite(out, stop.color[i]);
            }
        }
        IOUtilities::encodeAndWrite(out, shader.palette.stopEasing);
        IOUtilities::encodeAndWrite(out, uint32_t(s.reliefAutoZoom));
        IOUtilities::encodeAndWrite(out, s.reliefZoomReference);
    }

    void ShaderPresetIO::readSurface(std::ifstream &in, ShaderAttribute &shader) {
        auto &s = shader.slope;
        s.iridescence = 0.0f;
        s.filmThickness = 400.0f;
        s.specularAA = 0.0f;
        s.lustreRelief = false;
        s.reliefDepth = 1.0f;
        s.normalSmooth = 0.0f;
        s.aoRadius = 8.0f;
        s.reliefWaves = 0.0f;
        s.waveFrequency = 0.5f;
        s.invertRelief = false;
        s.boundaryGuard = 0.0f;
        s.reliefAutoZoom = true;
        s.reliefZoomReference = -1.0f;
        shader.palette.stops.clear();
        shader.palette.stopEasing = 1;
        auto hasMore = [&in] { return !in.fail() && in.rdbuf()->sgetc() != std::char_traits<char>::eof(); };
        if (!hasMore()) {
            return;
        }
        uint32_t magic = 0;
        IOUtilities::readAndDecode(in, &magic);
        if (in.fail() || magic != 0x53524631) {
            return;
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.iridescence);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.filmThickness);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.specularAA);
        }
        if (hasMore()) {
            uint32_t flag = 0;
            IOUtilities::readAndDecode(in, &flag);
            if (flag > 1) {
                in.setstate(std::ios::failbit);
            }
            s.lustreRelief = flag == 1;
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.reliefDepth);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.normalSmooth);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.aoRadius);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.reliefWaves);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.waveFrequency);
        }
        if (hasMore()) {
            uint32_t flag = 0;
            IOUtilities::readAndDecode(in, &flag);
            if (flag > 1) {
                in.setstate(std::ios::failbit);
            }
            s.invertRelief = flag == 1;
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.boundaryGuard);
        }
        if (!hasMore()) {
            return;
        }
        uint32_t count = 0;
        IOUtilities::readAndDecode(in, &count);
        if (count > 32 || count == 1) {
            in.setstate(std::ios::failbit);
            return;
        }
        shader.palette.stops.resize(count);
        for (auto &stop : shader.palette.stops) {
            IOUtilities::readAndDecode(in, &stop.position);
            for (int i = 0; i < 4; ++i) {
                IOUtilities::readAndDecode(in, &stop.color[i]);
            }
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &shader.palette.stopEasing);
        }
        if (hasMore()) {
            uint32_t flag = 0;
            IOUtilities::readAndDecode(in, &flag);
            if (flag > 1) {
                in.setstate(std::ios::failbit);
            }
            s.reliefAutoZoom = flag == 1;
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.reliefZoomReference);
        }
    }

    bool ShaderPresetIO::save(const std::filesystem::path &path, const ShaderAttribute &shader) {
        const std::filesystem::path temporary = IOUtilities::temporaryFilePath(path);
        std::ofstream out(temporary, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            vkh::logger::w_log(L"ERROR : Cannot save shader preset");
            return false;
        }
        IOUtilities::encodeAndWrite(out, MAGIC);
        IOUtilities::encodeAndWrite(out, VERSION);
        writeShader(out, shader);
        writeAnimationShape(out, shader);
        // Appended last: slope dual-scale relief (N3). Older presets lack these and fall back to defaults.
        IOUtilities::encodeAndWrite(out, shader.slope.macroRelief);
        IOUtilities::encodeAndWrite(out, shader.slope.macroRadius);
        writeTexture(out, shader, 0);
        // Appended last: generated pattern block. Older presets lack it and fall back to defaults.
        writePattern(out, shader);
        // Appended last: fog rim mask. Older presets lack it and fall back to the unmasked fog.
        IOUtilities::encodeAndWrite(out, shader.fog.rimMask);
        IOUtilities::encodeAndWrite(out, shader.fog.rimMaskBoost);
        IOUtilities::encodeAndWrite(out, shader.fog.rimBlur);
        IOUtilities::encodeAndWrite(out, shader.fog.centerStart);
        IOUtilities::encodeAndWrite(out, shader.fog.centerInvert);
        // Appended last: texture layers 1 and up. Older presets lack them and keep the defaults.
        writeExtraTextureLayers(out, shader);
        // Appended last: the OKLab blend choices. Older presets lack them and keep the original blends.
        writeOklabModes(out, shader);
        // Appended last: the domain warp. Older presets lack it and keep the coloring unwarped.
        writeWarp(out, shader);
        // Appended last: the slope lighting controls. Older presets lack these and take the
        // defaults, every one of which is the behaviour those presets were written under.
        IOUtilities::encodeAndWrite(out, shader.slope.reliefResponse);
        IOUtilities::encodeAndWrite(out, shader.slope.terminatorSoftness);
        IOUtilities::encodeAndWrite(out, shader.slope.highlightKnee);
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(shader.slope.lightBlend));
        // Appended last: the pattern's outline. Older presets lack it and keep the pattern unoutlined.
        writePatternEdge(out, shader);
        // Appended last: the chromatic shading controls, the fog's focus band, and whether the
        // outline's width is relative. Older presets lack these and take the defaults, every one of
        // which is the behaviour those presets were written under.
        writeTrailer(out, shader);
        // Appended last, outside the trailer: the config stream puts its timeline block after that
        // block, so a field inside it would be read out of an older config's timeline marker.
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(shader.fog.blurQuality));
        // Appended last: the HDR block. Older presets lack it and load with HDR off.
        writeHdr(out, shader);
        // Appended last: the palette's band lines. Older presets lack them and load with them off.
        writeBandLine(out, shader);
        // Appended last: the slope's fill light. Older presets lack it and load with the single light.
        IOUtilities::encodeAndWrite(out, shader.slope.fillIntensity);
        IOUtilities::encodeAndWrite(out, shader.slope.fillZenith);
        IOUtilities::encodeAndWrite(out, shader.slope.fillAzimuth);
        // Appended last: the palette's cycle bias. Older presets lack it and load at the straight mapping.
        IOUtilities::encodeAndWrite(out, shader.palette.cycleBias);
        // Appended last: the palette's cycle curve choice. Older presets lack it and load on Power.
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(shader.palette.cycleCurve));
        // Appended last: the bloom's linear sum. Older presets lack it and load on the encoded one they were written under.
        IOUtilities::encodeAndWrite(out, shader.bloom.linearAdd);
        // Appended last: the layers' Size and Keep Aspect. Older presets lack them and load stretched to a square tile, as they were written.
        writeTextureSize(out, shader);
        // Appended last, behind a marker of its own: the slope's gloss. Older presets lack it and
        // load with the gloss off, which is the picture they were written under.
        writeGloss(out, shader);
        // Appended last, behind a marker of its own: the palette's iteration coloring. Older presets lack it and load on the straight count they were written under.
        writePaletteColoring(out, shader);
        // Appended last, behind a marker of its own: the gloss's Relief. Older presets lack it and load on its default.
        writeGlossRelief(out, shader);
        writeStudio(out, shader);
        writeSurface(out, shader);
        writeEffects(out, shader);
        writeGroove(out, shader);
        writeEffectSync(out, shader);
        writeChrome(out, shader);
        IOUtilities::encodeAndWrite(out, shader.hdr.mfrPeakNits);
        writeLayerOrder(out, shader);
        writeStudioLighting(out, shader);
        writeBandDecorations(out, shader);
        writeSurfaceReplacement(out, shader);
        writeChaosBlur(out, shader);
        out.close();
        if (out.fail()) {
            IOUtilities::discardTemporaryFile(temporary);
            vkh::logger::w_log(L"ERROR : Cannot save shader preset");
            return false;
        }
        if (!IOUtilities::commitTemporaryFile(temporary, path)) {
            IOUtilities::discardTemporaryFile(temporary);
            vkh::logger::w_log(L"ERROR : Cannot replace shader preset");
            return false;
        }
        return true;
    }

    bool ShaderPresetIO::load(const std::filesystem::path &path, ShaderAttribute &out) {
        if (!std::filesystem::exists(path)) {
            return false;
        }
        std::ifstream in(path, std::ios::in | std::ios::binary);
        if (!in.is_open()) {
            return false;
        }
        uint32_t magic;
        uint32_t version;
        IOUtilities::readAndDecode(in, &magic);
        IOUtilities::readAndDecode(in, &version);
        if (magic != MAGIC || version < 1 || version > VERSION) {
            vkh::logger::w_log(L"ERROR : Not a valid shader preset file");
            return false;
        }
        ShaderAttribute s = {};
        // A preset carrying no Blur Quality block was written under the 16-texel ceiling, so it is
        // read back under it rather than under the Appearance a fresh session starts on. The block
        // below overwrites this when the preset does carry one.
        s.fog.blurQuality = ShdFogBlurQuality::SPEED;
        readShader(in, s, version >= 2, version >= 3, version < 4);
        auto hasMore = [&in] { return in.rdbuf()->sgetc() != std::char_traits<char>::eof(); };
        if (hasMore()) {
            readAnimationShape(in, s);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.slope.macroRelief);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.slope.macroRadius);
        }
        if (hasMore()) {
            readTexture(in, s, 0);
        }
        if (hasMore()) {
            readPattern(in, s);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.fog.rimMask);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.fog.rimMaskBoost);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.fog.rimBlur);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.fog.centerStart);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.fog.centerInvert);
        }
        if (hasMore()) {
            readExtraTextureLayers(in, s);
        }
        if (hasMore()) {
            readOklabModes(in, s);
        }
        if (hasMore()) {
            readWarp(in, s);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.slope.reliefResponse);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.slope.terminatorSoftness);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.slope.highlightKnee);
        }
        if (hasMore()) {
            int32_t lightBlend;
            IOUtilities::readAndDecode(in, &lightBlend);
            // A preset may name a mode this build no longer has; fall back to the original composite.
            s.slope.lightBlend = lightBlend == 1 ? ShdSlopeLightBlend::LINEAR : ShdSlopeLightBlend::DIRECT;
        }
        if (hasMore()) {
            readPatternEdge(in, s);
        }
        if (hasMore()) {
            readTrailer(in, s);
        }
        if (hasMore()) {
            s.fog.blurQuality = readFogBlurQuality(in);
        }
        if (hasMore()) {
            readHdr(in, s);
        }
        if (hasMore()) {
            readBandLine(in, s);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.slope.fillIntensity);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.slope.fillZenith);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.slope.fillAzimuth);
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.palette.cycleBias);
        }
        if (hasMore()) {
            int32_t cycleCurve;
            IOUtilities::readAndDecode(in, &cycleCurve);
            // A preset may name a curve this build no longer has; fall back to the power mapping.
            s.palette.cycleCurve = cycleCurve == 1 ? ShdPaletteCycleCurve::WAVE : ShdPaletteCycleCurve::POWER;
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.bloom.linearAdd);
        }
        if (version < 4 && hasMore()) {
            float removedBandLineDepth;
            IOUtilities::readAndDecode(in, &removedBandLineDepth);
        }
        if (hasMore()) {
            readTextureSize(in, s);
        } else {
            clearLegacyTextureSize(s);
        }
        if (hasMore()) {
            readGloss(in, s);
        }
        if (hasMore()) {
            readPaletteColoring(in, s);
        }
        if (hasMore()) {
            readGlossRelief(in, s);
        }
        readStudio(in, s, version < 4);
        readSurface(in, s);
        readEffects(in, s);
        readGroove(in, s);
        readEffectSync(in, s);
        readChrome(in, s, version < 4);
        s.hdr.mfrPeakNits = 4000.0f;
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.hdr.mfrPeakNits);
        }
        readLayerOrder(in, s);
        readStudioLighting(in, s, version < 4);
        readBandDecorations(in, s);
        readSurfaceReplacement(in, s);
        readChaosBlur(in, s);
        if (in.fail() || !validate(s)) {
            vkh::logger::w_log(L"ERROR : Shader preset file is corrupted");
            return false;
        }
        out = std::move(s);
        return true;
    }

    std::vector<std::wstring> ShaderPresetIO::missingTextureImages(const ShaderAttribute &shader) {
        std::vector<std::wstring> missing;
        for (uint32_t layer = 0; layer < TEXTURE_LAYER_COUNT; ++layer) {
            const auto &path = shader.textures[layer].path;
            if (path.empty()) {
                continue;
            }
            // The path is stored as UTF-8; widen it the way the texture uploader does, so a
            // non-ASCII name is tested as the same file it will later be read from.
            const std::filesystem::path fsPath{
                std::u8string(reinterpret_cast<const char8_t *>(path.data()), path.size())};
            std::error_code ec;
            if (!std::filesystem::exists(fsPath, ec)) {
                missing.push_back(std::format(L"Layer {}: {}", layer + 1, fsPath.wstring()));
            }
        }
        return missing;
    }
} // namespace merutilm::rff2
