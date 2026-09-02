//
// Created and modified by AI; earlier exact dates unavailable.
// Modified by GPT-5 on 2026-08-16, 2026-08-21, 2026-08-23, 2026-08-27, 2026-08-31, 2026-09-01
// Modified by Opus 4.8 on 2026-07-05
// Modified by Opus 5 on 2026-08-05, 2026-08-07, 2026-08-08, 2026-08-13, 2026-08-14, 2026-08-15, 2026-08-16, 2026-08-17, 2026-08-19, 2026-08-20, 2026-08-22, 2026-08-24, 2026-08-27, 2026-08-29, 2026-08-31
// Modified by ox-alpha on 2026-08-22.
// Modified by Fable 5.1 on 2026-09-02
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
        constexpr uint64_t MAX_PALETTE_COLORS = 1ULL << 20;
        constexpr uint64_t MAX_TEXTURE_PATH_BYTES = 1024ULL * 1024;

        bool finiteVec4(const glm::vec4 &v) {
            return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z) && std::isfinite(v.w);
        }

        template<typename E>
        bool enumInRange(const E value, const int32_t min, const int32_t max) {
            const int32_t raw = static_cast<int32_t>(value);
            return raw >= min && raw <= max;
        }
    }

    bool ShaderPresetIO::validate(const ShaderAttribute &shader) {
        const auto finite = [](const std::initializer_list<float> values) {
            return std::ranges::all_of(values, [](const float value) { return std::isfinite(value); });
        };
        const auto &p = shader.palette;
        if (p.colors.empty() || p.colors.size() > MAX_PALETTE_COLORS ||
            !std::ranges::all_of(p.colors, finiteVec4) || !finiteVec4(p.iterationInterval) ||
            p.iterationInterval.x <= 0.0f || p.iterationInterval.y <= 0.0f ||
            p.iterationInterval.z <= 0.0f || p.iterationInterval.w <= 0.0f ||
            !finite({p.offsetRatio, p.animationSpeed, p.animationFlowAmount, p.animationFlowScale,
                     p.animationFlowSpeed, p.animationFlowSwirl, p.staticColorTolerance, p.cycleBias,
                     p.bandLineWidth, p.bandLineOpacity, p.bandLineSoftness}) ||
            !finiteVec4(p.glossColor) || !finiteVec4(p.mandelbrotColor) || !finiteVec4(p.bandLineColor) ||
            !std::ranges::all_of(p.staticColorIterations, [](const double value) { return std::isfinite(value); }) ||
            !enumInRange(p.colorSmoothing, 0, 2) || !enumInRange(p.colorInterpolation, 0, 1) ||
            !enumInRange(p.cycleCurve, 0, 1) || !enumInRange(p.iterationColoring, 0, 6) ||
            !(p.animationMode == ShdPaletteAnimationMode::LINEAR ||
              p.animationMode == ShdPaletteAnimationMode::PSYCHEDELIC ||
              p.animationMode == ShdPaletteAnimationMode::BREATHING ||
              p.animationMode == ShdPaletteAnimationMode::TURBULENCE)) {
            return false;
        }

        const auto &stripe = shader.stripe;
        if (!finite({stripe.firstInterval, stripe.secondInterval, stripe.opacity, stripe.offset,
                     stripe.animationSpeed}) || !enumInRange(stripe.stripeType, 0, 3)) {
            return false;
        }

        const auto &s = shader.slope;
        if (!finite({s.depth, s.reflectionRatio, s.opacity, s.zenith, s.azimuth, s.specularIntensity,
                     s.specularPower, s.rimIntensity, s.rimPower, s.brightness, s.gamma, s.aoIntensity,
                     s.ambientIntensity, s.specularZenith, s.specularAzimuth, s.specularAnisotropy,
                     s.specularAnisotropyAngle, s.macroRelief, s.macroRadius, s.reliefResponse,
                     s.terminatorSoftness, s.highlightKnee, s.lumaAmount, s.tintResponse, s.shadowChroma,
                     s.fillIntensity, s.fillZenith, s.fillAzimuth, s.glossIntensity, s.glossBands,
                     s.glossSharpness, s.glossPhase, s.glossRelief}) || !finiteVec4(s.rimColor) ||
            !finiteVec4(s.specularColor) || !finiteVec4(s.skyColor) || !finiteVec4(s.groundColor) ||
            !finiteVec4(s.glossColor) ||
            !(s.shadingBlend == ShdSlopeShadingBlend::OVERLAY ||
              s.shadingBlend == ShdSlopeShadingBlend::OKLAB_LIGHTNESS) ||
            !enumInRange(s.lightBlend, 0, 1) || !enumInRange(s.tintBlend, 0, 1) ||
            !enumInRange(s.glossSource, 0, 3)) {
            return false;
        }

        const auto &color = shader.color;
        const auto &fog = shader.fog;
        const auto &bloom = shader.bloom;
        if (!finite({color.gamma, color.exposure, color.hue, color.saturation, color.brightness,
                     color.contrast, fog.radius, fog.opacity, fog.centerStart, fog.rimMask,
                     fog.rimMaskBoost, fog.rimBlur, fog.focusAmount, fog.focusRatio, fog.focusRange,
                     fog.focusFalloff, fog.focusBlur, bloom.threshold, bloom.radius, bloom.softness,
                     bloom.intensity}) || !enumInRange(fog.blurQuality, 0, 1)) {
            return false;
        }

        for (const auto &texture : shader.textures) {
            if (!finite({texture.opacity, texture.scaleU, texture.scaleV, texture.scrollU, texture.scrollV,
                         texture.paletteFollow, texture.periodIterations, texture.size}) ||
                !enumInRange(texture.uvMode, 0, 3) || !enumInRange(texture.blendMode, 0, 2)) {
                return false;
            }
        }
        for (const auto &pattern : shader.patterns) {
            if (!finite({pattern.opacity, pattern.paletteShift, pattern.sharpness, pattern.scaleU,
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
        return finite({warp.amount, warp.octaves, warp.scaleU, warp.scaleV, warp.scrollU, warp.scrollV,
                       warp.paletteFollow, warp.periodIterations, hdr.exposure, hdr.headroom}) &&
               enumInRange(warp.source, 0, 4) && enumInRange(warp.uvMode, 0, 3) &&
               enumInRange(hdr.method, 0, 3);
    }

    void ShaderPresetIO::writeShader(std::ofstream &out, const ShaderAttribute &shader) {
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
        // Slope shadow was removed, but its two floats stay in the stream so the flat layout never shifts.
        IOUtilities::encodeAndWrite(out, 0.0f);
        IOUtilities::encodeAndWrite(out, 1.0f);
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
                                    const bool hasFrozenColors) {
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
                if (!IOUtilities::validateReadCount(in, colorCount, sizeof(float) * 3,
                                                    MAX_PALETTE_COLORS)) {
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
            if (!IOUtilities::validateReadCount(in, colorCount, sizeof(float) * 4,
                                                MAX_PALETTE_COLORS)) {
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
        float removedSlopeShadow = 0.0f;
        IOUtilities::readAndDecode(in, &removedSlopeShadow);
        IOUtilities::readAndDecode(in, &removedSlopeShadow);
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

    void ShaderPresetIO::writeAnimationShape(std::ofstream &out, const ShaderAttribute &shader) {
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

    void ShaderPresetIO::writeTexture(std::ofstream &out, const ShaderAttribute &shader, const uint32_t layer) {
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
        t.uvMode = uvMode >= 0 && uvMode <= 3 ? static_cast<ShdTextureUVMode>(uvMode) : ShdTextureUVMode::CYCLE_BAND;
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

    void ShaderPresetIO::writeTextureSize(std::ofstream &out, const ShaderAttribute &shader) {
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

    void ShaderPresetIO::writeExtraTextureLayers(std::ofstream &out, const ShaderAttribute &shader) {
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

    void ShaderPresetIO::writePattern(std::ofstream &out, const ShaderAttribute &shader) {
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
        p.uvMode = uvMode >= 0 && uvMode <= 3 ? static_cast<ShdTextureUVMode>(uvMode) : ShdTextureUVMode::CYCLE_BAND;
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
            p.inkMode = inkMode >= 0 && inkMode <= 1
                            ? static_cast<ShdPatternInkMode>(inkMode)
                            : ShdPatternInkMode::PALETTE_SHIFT;
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &p.paletteShift);
        }
    }

    void ShaderPresetIO::writeOklabModes(std::ofstream &out, const ShaderAttribute &shader) {
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(shader.palette.colorInterpolation));
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(shader.slope.shadingBlend));
    }

    void ShaderPresetIO::readOklabModes(std::ifstream &in, ShaderAttribute &out) {
        // Guarded one field at a time, so a file written between the two still loads.
        auto hasMore = [&in] { return in.rdbuf()->sgetc() != std::char_traits<char>::eof(); };
        if (hasMore()) {
            int32_t interpolation;
            IOUtilities::readAndDecode(in, &interpolation);
            // A file may name a mode this build no longer has; fall back rather than leave the radio group empty.
            out.palette.colorInterpolation = interpolation == 1
                                                 ? ShdPalColorInterpolationMethod::OKLAB
                                                 : ShdPalColorInterpolationMethod::RGB;
        }
        if (hasMore()) {
            int32_t shadingBlend;
            IOUtilities::readAndDecode(in, &shadingBlend);
            out.slope.shadingBlend = shadingBlend == 2
                                         ? ShdSlopeShadingBlend::OKLAB_LIGHTNESS
                                         : ShdSlopeShadingBlend::OVERLAY;
        }
    }

    void ShaderPresetIO::writeWarp(std::ofstream &out, const ShaderAttribute &shader) {
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
            w.uvMode = uvMode >= 0 && uvMode <= 3
                           ? static_cast<ShdTextureUVMode>(uvMode)
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

    void ShaderPresetIO::writePatternEdge(std::ofstream &out, const ShaderAttribute &shader) {
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

        void writeChromaticShading(std::ofstream &out, const ShaderAttribute &shader) {
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

        void writeFocusBand(std::ofstream &out, const ShaderAttribute &shader) {
            const auto &f = shader.fog;
            IOUtilities::encodeAndWrite(out, f.focusAmount);
            IOUtilities::encodeAndWrite(out, f.focusRatio);
            IOUtilities::encodeAndWrite(out, f.focusRange);
            IOUtilities::encodeAndWrite(out, f.focusFalloff);
            IOUtilities::encodeAndWrite(out, f.focusBlur);
        }

        // One whole pattern layer, in the order the shader's own layer block is laid out.
        void writePatternLayer(std::ofstream &out, const ShdPatternAttribute &p) {
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
            p.uvMode = uvMode >= 0 && uvMode <= 3
                           ? static_cast<ShdTextureUVMode>(uvMode)
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
            p.inkMode = inkMode >= 0 && inkMode <= 1
                            ? static_cast<ShdPatternInkMode>(inkMode)
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
    }

    void ShaderPresetIO::writeTrailer(std::ofstream &out, const ShaderAttribute &shader) {
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

    void ShaderPresetIO::writeHdr(std::ofstream &out, const ShaderAttribute &shader) {
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
            h.method = method >= 0 && method <= 3
                           ? static_cast<ShdToneMapMethod>(method)
                           : ShdToneMapMethod::CLIP;
        }
    }

    void ShaderPresetIO::writeBandLine(std::ofstream &out, const ShaderAttribute &shader) {
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

    void ShaderPresetIO::writeGloss(std::ofstream &out, const ShaderAttribute &shader) {
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
                s.glossSource = source >= 0 && source <= 3
                                    ? static_cast<ShdSlopeGlossSource>(source)
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

    void ShaderPresetIO::writePaletteColoring(std::ofstream &out, const ShaderAttribute &shader) {
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

    void ShaderPresetIO::writeGlossRelief(std::ofstream &out, const ShaderAttribute &shader) {
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
        // Retained as zero so presets from the Line Depth prototype keep every later field aligned.
        IOUtilities::encodeAndWrite(out, 0.0f);
        // Appended last: the layers' Size and Keep Aspect. Older presets lack them and load stretched to a square tile, as they were written.
        writeTextureSize(out, shader);
        // Appended last, behind a marker of its own: the slope's gloss. Older presets lack it and
        // load with the gloss off, which is the picture they were written under.
        writeGloss(out, shader);
        // Appended last, behind a marker of its own: the palette's iteration coloring. Older presets lack it and load on the straight count they were written under.
        writePaletteColoring(out, shader);
        // Appended last, behind a marker of its own: the gloss's Relief. Older presets lack it and load on its default.
        writeGlossRelief(out, shader);
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
        if (magic != MAGIC || version > VERSION) {
            vkh::logger::w_log(L"ERROR : Not a valid shader preset file");
            return false;
        }
        ShaderAttribute s = {};
        // A preset carrying no Blur Quality block was written under the 16-texel ceiling, so it is
        // read back under it rather than under the Appearance a fresh session starts on. The block
        // below overwrites this when the preset does carry one.
        s.fog.blurQuality = ShdFogBlurQuality::SPEED;
        readShader(in, s, version >= 2, version >= 3);
        auto hasMore = [&in] {
            return in.rdbuf()->sgetc() != std::char_traits<char>::eof();
        };
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
            s.slope.lightBlend = lightBlend == 1
                                     ? ShdSlopeLightBlend::LINEAR
                                     : ShdSlopeLightBlend::DIRECT;
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
            s.palette.cycleCurve = cycleCurve == 1
                                       ? ShdPaletteCycleCurve::WAVE
                                       : ShdPaletteCycleCurve::POWER;
        }
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &s.bloom.linearAdd);
        }
        if (hasMore()) {
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
                std::u8string(reinterpret_cast<const char8_t *>(path.data()), path.size())
            };
            std::error_code ec;
            if (!std::filesystem::exists(fsPath, ec)) {
                missing.push_back(std::format(L"Layer {}: {}", layer + 1, fsPath.wstring()));
            }
        }
        return missing;
    }
}
