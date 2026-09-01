//
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21, 2026-08-27.
// Modified by Opus 5 on 2026-08-15, 2026-08-20, 2026-08-21, 2026-08-22, 2026-08-31
// Modified by ox-alpha on 2026-08-22.
//

#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>

#include "ShdPaletteCycleCurve.h"
#include "ShdPalIterationColoringMode.h"
#include "ShdPaletteAnimationMode.h"
#include "ShdPalColorSmoothingMethod.h"
#include "ShdPalColorInterpolationMethod.h"


namespace merutilm::rff2 {
    // sRGB <-> OKLab on vk_iteration_palette.frag's constants, so a blend done on the host lands
    // where the shader's does. Shared by the settings preview and the palette upload.
    // Matrices from Björn Ottosson, "A perceptual color space for image processing" (2020), published as MIT / public domain.
    inline glm::vec3 paletteLinearToOklab(const glm::vec3 &c) {
        const float l = 0.4122214708f * c.r + 0.5363325363f * c.g + 0.0514459929f * c.b;
        const float m = 0.2119034982f * c.r + 0.6806995451f * c.g + 0.1073969566f * c.b;
        const float s = 0.0883024619f * c.r + 0.2817188376f * c.g + 0.6299787005f * c.b;
        const float l_ = std::cbrt(std::max(l, 0.0f));
        const float m_ = std::cbrt(std::max(m, 0.0f));
        const float s_ = std::cbrt(std::max(s, 0.0f));
        return {
            0.2104542553f * l_ + 0.7936177850f * m_ - 0.0040720468f * s_,
            1.9779984951f * l_ - 2.4285922050f * m_ + 0.4505937099f * s_,
            0.0259040371f * l_ + 0.7827717662f * m_ - 0.8086757660f * s_
        };
    }

    inline glm::vec3 paletteOklabToLinear(const glm::vec3 &lab) {
        const float l_ = lab.x + 0.3963377774f * lab.y + 0.2158037573f * lab.z;
        const float m_ = lab.x - 0.1055613458f * lab.y - 0.0638541728f * lab.z;
        const float s_ = lab.x - 0.0894841775f * lab.y - 1.2914855480f * lab.z;
        const float l = l_ * l_ * l_;
        const float m = m_ * m_ * m_;
        const float s = s_ * s_ * s_;
        return {
            4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s,
            -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s,
            -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s
        };
    }

    inline glm::vec4 blendPaletteColors(const glm::vec4 &c0, const glm::vec4 &c1, const float t,
                                        const ShdPalColorInterpolationMethod interp) {
        if (interp == ShdPalColorInterpolationMethod::RGB) {
            return glm::mix(c0, c1, t);
        }
        auto toLinear = [](const glm::vec4 &c) {
            return glm::vec3(std::pow(std::max(c.r, 0.0f), 2.2f),
                             std::pow(std::max(c.g, 0.0f), 2.2f),
                             std::pow(std::max(c.b, 0.0f), 2.2f));
        };
        const glm::vec3 lab = glm::mix(paletteLinearToOklab(toLinear(c0)), paletteLinearToOklab(toLinear(c1)), t);
        const glm::vec3 lin = paletteOklabToLinear(lab);
        return {
            std::clamp(std::pow(std::max(lin.r, 0.0f), 1.0f / 2.2f), 0.0f, 1.0f),
            std::clamp(std::pow(std::max(lin.g, 0.0f), 1.0f / 2.2f), 0.0f, 1.0f),
            std::clamp(std::pow(std::max(lin.b, 0.0f), 1.0f / 2.2f), 0.0f, 1.0f),
            std::lerp(c0.a, c1.a, t)
        };
    }

    struct ShdPaletteAttribute {
        // Upper bound on eyedropper-frozen colors (must match the array size in the palette SSBO/shaders).
        static constexpr uint32_t MAX_STATIC_COLORS = 16;

        std::vector<glm::vec4> colors;
        ShdPalColorSmoothingMethod colorSmoothing;
        // Blend space between two adjacent palette entries. RGB is the original mix.
        ShdPalColorInterpolationMethod colorInterpolation = ShdPalColorInterpolationMethod::RGB;
        glm::vec4 iterationInterval;
        float offsetRatio;
        float animationSpeed;
        ShdPaletteAnimationMode animationMode = ShdPaletteAnimationMode::LINEAR;
        float animationFlowAmount = 80.0f;
        float animationFlowScale = 3.0f;
        float animationFlowSpeed = 0.5f;
        float animationFlowSwirl = 0.4f;
        bool enableGloss = false;
        glm::vec4 glossColor = {1.0f, 1.0f, 1.0f, 1.0f}; // Color of the gloss highlight (default: white)
        bool seamless = false;
        glm::vec4 mandelbrotColor = {0.0f, 0.0f, 0.0f, 1.0f}; // Color for Mandelbrot interior (iteration >= max)

        // Band Line: a line laid across the palette at even points of its cycle, so the color
        // bands are separated instead of running into one another.
        bool bandLineEnabled = false;
        // Lines in one full cycle. Matching the color count puts one on every color boundary.
        uint32_t bandLineCount = 16;
        // Thickness as a fraction of one band, measured across the boundary the line sits on.
        float bandLineWidth = 0.03f;
        float bandLineOpacity = 1.0f;
        // Feathering of the edges, as the fraction of the line's half-width the ramp takes up.
        // 0 leaves the line hard-edged; 1 turns it into one smooth swell with no edge at all.
        float bandLineSoftness = 0.0f;
        glm::vec4 bandLineColor = {0.0f, 0.0f, 0.0f, 1.0f};

        // Eyedropper-picked iteration values whose color is held static (animation disabled).
        std::vector<double> staticColorIterations;
        // Match radius in palette-cycle units (0..1) for deciding which pixels share a frozen color.
        float staticColorTolerance = 0.02f;

        // Recipe metadata: when >= 0 the color array was produced by this preset id with recipeSeed,
        // so saves store only {id, seed} and regenerate on load instead of dumping the full color array.
        int32_t recipePresetId = -1;
        uint32_t recipeSeed = 0;

        // Cycle Bias: pow curve on the cycle position, reshaping where a cycle spends its colors.
        // 1.0 is the straight mapping every earlier version drew.
        float cycleBias = 1.0f;
        // Which curve carries the bias. Power is the original pow mapping; Wave tilts the density
        // sinusoidally with the band widths bounded, so animated boundaries stay inside one speed range.
        ShdPaletteCycleCurve cycleCurve = ShdPaletteCycleCurve::POWER;

        // Iteration Coloring: the curve the iteration count is read through before it lands on the
        // cycle, widening the bands as the count climbs. Linear is the straight count every earlier
        // version drew. Each curve is normalized so one Cycle Length is still the first cycle's width.
        ShdPalIterationColoringMode iterationColoring = ShdPalIterationColoringMode::LINEAR;

        glm::vec4 getMidColor(float rat) const;

        // How strongly the band line covers that position of the palette cycle, 0 = not on a line.
        [[nodiscard]] float bandLineCoverage(float cycleRatio) const;
    };

    inline float ShdPaletteAttribute::bandLineCoverage(const float cycleRatio) const {
        if (!bandLineEnabled || bandLineCount == 0 || bandLineWidth <= 0.0f) {
            return 0.0f;
        }
        float f = std::fmod(cycleRatio * static_cast<float>(bandLineCount), 1.0f);
        if (f < 0.0f) {
            f += 1.0f;
        }
        // Distance to the nearest boundary as a fraction of one band; the line straddles it.
        const float d = std::min(f, 1.0f - f);
        // Where this sits across the line: 0 on the boundary it straddles, 1 at its outer edge.
        const float x = d * 2.0f / bandLineWidth;
        const float opacity = std::clamp(bandLineOpacity, 0.0f, 1.0f);
        const float softness = std::clamp(bandLineSoftness, 0.0f, 1.0f);
        if (softness <= 0.0f) {
            return x < 1.0f ? opacity : 0.0f;
        }
        // Feathered over the outer `softness` of the line, on a smoothstep shoulder so the ramp
        // leaves the color and arrives at the line without a corner at either end.
        const float featherStart = 1.0f - softness;
        const float featherEnd = 1.0f + softness;
        if (x <= featherStart) {
            return opacity;
        }
        if (x >= featherEnd) {
            return 0.0f;
        }
        const float t = 1.0f - (x - featherStart) / (featherEnd - featherStart);
        const float shoulder = t * t * (3.0f - 2.0f * t);
        const float shapedShoulder = shoulder * shoulder;
        return opacity * shapedShoulder * shapedShoulder;
    }

    inline glm::vec4 ShdPaletteAttribute::getMidColor(const float rat) const {

        auto get_val = [&](int channel, float val, float interval) {
             const float ratio = std::fmod(val / interval + offsetRatio, 1.0f);
             const float i = ratio * static_cast<float>(colors.size());
             const auto i0 = static_cast<int>(i);
             const auto i1 = i0 + 1;
             const float d = std::fmod(i, 1.0f);

             // handle wrap around
             const glm::vec4 &c1 = colors[i0 % colors.size()];
             const glm::vec4 &c2 = colors[i1 % colors.size()];

             // access specific channel
             return std::lerp(c1[channel], c2[channel], d);
        };

        // 'rat' passed to getMidColor seems to be treated as "iteration count" in original code?
        // "rat / iterationInterval"

        return glm::vec4{
            get_val(0, rat, iterationInterval.r),
            get_val(1, rat, iterationInterval.g),
            get_val(2, rat, iterationInterval.b),
            get_val(3, rat, iterationInterval.a)
        };
    }
}
