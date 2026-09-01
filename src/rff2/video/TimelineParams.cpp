// Modified by GPT-5 on 2026-08-18, 2026-08-23
// Modified by ox-alpha on 2026-08-22.
// Modified by Opus 5 on 2026-08-25, 2026-08-26, 2026-08-27, 2026-08-31, 2026-09-01

#include "TimelineParams.hpp"

#include <array>
#include <cmath>
#include <type_traits>

#include "../attr/VidTimelineTarget.h"

namespace merutilm::rff2 {
    namespace {
        template<auto Parent, auto Member>
        float getNestedValue(const ShaderAttribute &shader) {
            const auto &owner = shader.*Parent;
            using Value = std::remove_cvref_t<decltype(owner.*Member)>;
            if constexpr (std::is_same_v<Value, bool>) {
                return owner.*Member ? 1.0f : 0.0f;
            } else if constexpr (std::is_enum_v<Value>) {
                return static_cast<float>(static_cast<int32_t>(owner.*Member));
            } else {
                return static_cast<float>(owner.*Member);
            }
        }

        template<auto Parent, auto Member>
        void setNestedValue(ShaderAttribute &shader, const float value) {
            auto &owner = shader.*Parent;
            using Value = std::remove_cvref_t<decltype(owner.*Member)>;
            if constexpr (std::is_same_v<Value, bool>) {
                owner.*Member = value >= 0.5f;
            } else if constexpr (std::is_enum_v<Value>) {
                owner.*Member = static_cast<Value>(static_cast<int32_t>(std::lround(value)));
            } else {
                owner.*Member = static_cast<Value>(value);
            }
        }

        template<auto Parent, auto Member>
        const void *getNestedAddress(const ShaderAttribute &shader) {
            return &((shader.*Parent).*Member);
        }

        template<auto Parent, auto Member>
        glm::vec4 getNestedColor(const ShaderAttribute &shader) {
            return (shader.*Parent).*Member;
        }

        template<auto Parent, auto Member>
        void setNestedColor(ShaderAttribute &shader, const glm::vec4 &color) {
            (shader.*Parent).*Member = color;
        }

        template<size_t Component>
        float getPaletteInterval(const ShaderAttribute &shader) {
            return shader.palette.iterationInterval[Component];
        }

        template<size_t Component>
        void setPaletteInterval(ShaderAttribute &shader, const float value) {
            shader.palette.iterationInterval[Component] = value;
        }

        template<size_t Component>
        const void *getPaletteIntervalAddress(const ShaderAttribute &shader) {
            return &shader.palette.iterationInterval[Component];
        }

        template<size_t Layer, auto Member>
        const void *getTextureAddress(const ShaderAttribute &shader) {
            return &(shader.textures[Layer].*Member);
        }

        template<size_t Layer, auto Member>
        const void *getPatternAddress(const ShaderAttribute &shader) {
            return &(shader.patterns[Layer].*Member);
        }

        template<size_t Layer, auto Member>
        float getTextureValue(const ShaderAttribute &shader) {
            const auto &owner = shader.textures[Layer];
            using Value = std::remove_cvref_t<decltype(owner.*Member)>;
            if constexpr (std::is_same_v<Value, bool>) {
                return owner.*Member ? 1.0f : 0.0f;
            } else if constexpr (std::is_enum_v<Value>) {
                return static_cast<float>(static_cast<int32_t>(owner.*Member));
            } else {
                return static_cast<float>(owner.*Member);
            }
        }

        template<size_t Layer, auto Member>
        void setTextureValue(ShaderAttribute &shader, const float value) {
            auto &owner = shader.textures[Layer];
            using Value = std::remove_cvref_t<decltype(owner.*Member)>;
            if constexpr (std::is_same_v<Value, bool>) {
                owner.*Member = value >= 0.5f;
            } else if constexpr (std::is_enum_v<Value>) {
                owner.*Member = static_cast<Value>(static_cast<int32_t>(std::lround(value)));
            } else {
                owner.*Member = static_cast<Value>(value);
            }
        }

        template<size_t Layer, auto Member>
        float getPatternValue(const ShaderAttribute &shader) {
            const auto &owner = shader.patterns[Layer];
            using Value = std::remove_cvref_t<decltype(owner.*Member)>;
            if constexpr (std::is_same_v<Value, bool>) {
                return owner.*Member ? 1.0f : 0.0f;
            } else if constexpr (std::is_enum_v<Value>) {
                return static_cast<float>(static_cast<int32_t>(owner.*Member));
            } else {
                return static_cast<float>(owner.*Member);
            }
        }

        template<size_t Layer, auto Member>
        void setPatternValue(ShaderAttribute &shader, const float value) {
            auto &owner = shader.patterns[Layer];
            using Value = std::remove_cvref_t<decltype(owner.*Member)>;
            if constexpr (std::is_same_v<Value, bool>) {
                owner.*Member = value >= 0.5f;
            } else if constexpr (std::is_enum_v<Value>) {
                owner.*Member = static_cast<Value>(static_cast<int32_t>(std::lround(value)));
            } else {
                owner.*Member = static_cast<Value>(value);
            }
        }

        template<size_t Layer, auto Member>
        glm::vec4 getPatternColor(const ShaderAttribute &shader) {
            return shader.patterns[Layer].*Member;
        }

        template<size_t Layer, auto Member>
        void setPatternColor(ShaderAttribute &shader, const glm::vec4 &color) {
            shader.patterns[Layer].*Member = color;
        }

#define NESTED_VALUE(ID, GROUP, LABEL, KIND, DIRTY, MIN, MAX, PARENT, MEMBER) \
        {vidTimelineTargetId(VidTimelineTarget::ID), GROUP, LABEL, TimelineParamKind::KIND, \
         TimelineApplyCost::CHEAP, TimelineDirtyMask::DIRTY, MIN, MAX, \
         &getNestedValue<&ShaderAttribute::PARENT, &decltype(ShaderAttribute::PARENT)::MEMBER>, \
         &setNestedValue<&ShaderAttribute::PARENT, &decltype(ShaderAttribute::PARENT)::MEMBER>, nullptr, nullptr, \
         &getNestedAddress<&ShaderAttribute::PARENT, &decltype(ShaderAttribute::PARENT)::MEMBER>}

#define NESTED_COLOR(ID, GROUP, LABEL, DIRTY, PARENT, MEMBER) \
        {vidTimelineTargetId(VidTimelineTarget::ID), GROUP, LABEL, TimelineParamKind::COLOR, \
         TimelineApplyCost::CHEAP, TimelineDirtyMask::DIRTY, 0.0f, 1.0f, nullptr, nullptr, \
         &getNestedColor<&ShaderAttribute::PARENT, &decltype(ShaderAttribute::PARENT)::MEMBER>, \
         &setNestedColor<&ShaderAttribute::PARENT, &decltype(ShaderAttribute::PARENT)::MEMBER>, \
         &getNestedAddress<&ShaderAttribute::PARENT, &decltype(ShaderAttribute::PARENT)::MEMBER>}

#define WIDEN_INNER(VALUE) L##VALUE
#define WIDEN(VALUE) WIDEN_INNER(VALUE)

#define TEXTURE_VALUE(ID, LAYER, LABEL, KIND, MIN, MAX, MEMBER) \
        {vidTimelineTargetId(VidTimelineTarget::ID), L"Texture " WIDEN(#LAYER), LABEL, TimelineParamKind::KIND, \
         TimelineApplyCost::CHEAP, TimelineDirtyMask::TEXTURE, MIN, MAX, \
         &getTextureValue<LAYER - 1, &ShdTextureAttribute::MEMBER>, \
         &setTextureValue<LAYER - 1, &ShdTextureAttribute::MEMBER>, nullptr, nullptr, \
         &getTextureAddress<LAYER - 1, &ShdTextureAttribute::MEMBER>}

#define PATTERN_VALUE(ID, LAYER, LABEL, KIND, MIN, MAX, MEMBER) \
        {vidTimelineTargetId(VidTimelineTarget::ID), L"Pattern " WIDEN(#LAYER), LABEL, TimelineParamKind::KIND, \
         TimelineApplyCost::CHEAP, TimelineDirtyMask::PATTERN, MIN, MAX, \
         &getPatternValue<LAYER - 1, &ShdPatternAttribute::MEMBER>, \
         &setPatternValue<LAYER - 1, &ShdPatternAttribute::MEMBER>, nullptr, nullptr, \
         &getPatternAddress<LAYER - 1, &ShdPatternAttribute::MEMBER>}

#define PATTERN_COLOR(ID, LAYER, LABEL, MEMBER) \
        {vidTimelineTargetId(VidTimelineTarget::ID), L"Pattern " WIDEN(#LAYER), LABEL, TimelineParamKind::COLOR, \
         TimelineApplyCost::CHEAP, TimelineDirtyMask::PATTERN, 0.0f, 1.0f, nullptr, nullptr, \
         &getPatternColor<LAYER - 1, &ShdPatternAttribute::MEMBER>, \
         &setPatternColor<LAYER - 1, &ShdPatternAttribute::MEMBER>, \
         &getPatternAddress<LAYER - 1, &ShdPatternAttribute::MEMBER>}

#define TEXTURE_LAYER(N) \
        TEXTURE_VALUE(TEXTURE_##N##_ENABLED, N, L"Enabled", BOOL, 0.0f, 1.0f, enabled), \
        TEXTURE_VALUE(TEXTURE_##N##_UV_MODE, N, L"UV Mode", ENUM, 0.0f, 3.0f, uvMode), \
        TEXTURE_VALUE(TEXTURE_##N##_BLEND_MODE, N, L"Blend Mode", ENUM, 0.0f, 2.0f, blendMode), \
        TEXTURE_VALUE(TEXTURE_##N##_OPACITY, N, L"Opacity", FLOAT, 0.0f, 1.0f, opacity), \
        TEXTURE_VALUE(TEXTURE_##N##_SCALE_U, N, L"Scale U", FLOAT, 0.0f, 20.0f, scaleU), \
        TEXTURE_VALUE(TEXTURE_##N##_SCALE_V, N, L"Scale V", FLOAT, 0.0f, 20.0f, scaleV), \
        TEXTURE_VALUE(TEXTURE_##N##_SCROLL_U, N, L"Scroll U", FLOAT, -2.0f, 2.0f, scrollU), \
        TEXTURE_VALUE(TEXTURE_##N##_SCROLL_V, N, L"Scroll V", FLOAT, -2.0f, 2.0f, scrollV), \
        TEXTURE_VALUE(TEXTURE_##N##_PALETTE_FOLLOW, N, L"Palette Follow", FLOAT, -2.0f, 2.0f, paletteFollow), \
        TEXTURE_VALUE(TEXTURE_##N##_PERIOD, N, L"Period Iterations", FLOAT, 0.0f, 1000000000.0f, periodIterations), \
        TEXTURE_VALUE(TEXTURE_##N##_SIZE, N, L"Size", FLOAT, 0.1f, 20.0f, size), \
        TEXTURE_VALUE(TEXTURE_##N##_KEEP_ASPECT, N, L"Keep Aspect", BOOL, 0.0f, 1.0f, keepAspect)

#define PATTERN_LAYER(N) \
        PATTERN_VALUE(PATTERN_##N##_ENABLED, N, L"Enabled", BOOL, 0.0f, 1.0f, enabled), \
        PATTERN_VALUE(PATTERN_##N##_TYPE, N, L"Type", ENUM, 0.0f, 7.0f, type), \
        PATTERN_VALUE(PATTERN_##N##_UV_MODE, N, L"UV Mode", ENUM, 0.0f, 3.0f, uvMode), \
        PATTERN_VALUE(PATTERN_##N##_BLEND_MODE, N, L"Blend Mode", ENUM, 0.0f, 2.0f, blendMode), \
        PATTERN_VALUE(PATTERN_##N##_OPACITY, N, L"Opacity", FLOAT, 0.0f, 1.0f, opacity), \
        PATTERN_VALUE(PATTERN_##N##_INK_MODE, N, L"Ink Mode", ENUM, 0.0f, 1.0f, inkMode), \
        PATTERN_COLOR(PATTERN_##N##_COLOR, N, L"Color", color), \
        PATTERN_VALUE(PATTERN_##N##_PALETTE_SHIFT, N, L"Palette Shift", FLOAT, 0.0f, 1.0f, paletteShift), \
        PATTERN_VALUE(PATTERN_##N##_SHARPNESS, N, L"Sharpness", FLOAT, 0.0f, 1.0f, sharpness), \
        PATTERN_VALUE(PATTERN_##N##_SCALE_U, N, L"Scale U", FLOAT, 0.0f, 200.0f, scaleU), \
        PATTERN_VALUE(PATTERN_##N##_SCALE_V, N, L"Scale V", FLOAT, 0.0f, 200.0f, scaleV), \
        PATTERN_VALUE(PATTERN_##N##_SCROLL_U, N, L"Scroll U", FLOAT, -2.0f, 2.0f, scrollU), \
        PATTERN_VALUE(PATTERN_##N##_SCROLL_V, N, L"Scroll V", FLOAT, -2.0f, 2.0f, scrollV), \
        PATTERN_VALUE(PATTERN_##N##_PALETTE_FOLLOW, N, L"Palette Follow", FLOAT, -2.0f, 2.0f, paletteFollow), \
        PATTERN_VALUE(PATTERN_##N##_PERIOD, N, L"Period Iterations", FLOAT, 0.0f, 1000000000.0f, periodIterations), \
        PATTERN_VALUE(PATTERN_##N##_EDGE_ENABLED, N, L"Edge Enabled", BOOL, 0.0f, 1.0f, edgeEnabled), \
        PATTERN_COLOR(PATTERN_##N##_EDGE_COLOR, N, L"Edge Color", edgeColor), \
        PATTERN_VALUE(PATTERN_##N##_EDGE_WIDTH, N, L"Edge Width", FLOAT, -0.5f, 1.0f, edgeWidth), \
        PATTERN_VALUE(PATTERN_##N##_EDGE_OPACITY, N, L"Edge Opacity", FLOAT, 0.0f, 1.0f, edgeOpacity), \
        PATTERN_VALUE(PATTERN_##N##_EDGE_RELATIVE, N, L"Edge Relative", BOOL, 0.0f, 1.0f, edgeRelative)

        const auto PARAMS = std::to_array<TimelineParamDesc>({
            {vidTimelineTargetId(VidTimelineTarget::PALETTE_INTERVAL_R), L"Palette", L"Iteration Interval R", TimelineParamKind::FLOAT, TimelineApplyCost::CHEAP, TimelineDirtyMask::PALETTE, 1.0f, 1.0e18f, &getPaletteInterval<0>, &setPaletteInterval<0>, nullptr, nullptr, &getPaletteIntervalAddress<0>},
            {vidTimelineTargetId(VidTimelineTarget::PALETTE_INTERVAL_G), L"Palette", L"Iteration Interval G", TimelineParamKind::FLOAT, TimelineApplyCost::CHEAP, TimelineDirtyMask::PALETTE, 1.0f, 1.0e18f, &getPaletteInterval<1>, &setPaletteInterval<1>, nullptr, nullptr, &getPaletteIntervalAddress<1>},
            {vidTimelineTargetId(VidTimelineTarget::PALETTE_INTERVAL_B), L"Palette", L"Iteration Interval B", TimelineParamKind::FLOAT, TimelineApplyCost::CHEAP, TimelineDirtyMask::PALETTE, 1.0f, 1.0e18f, &getPaletteInterval<2>, &setPaletteInterval<2>, nullptr, nullptr, &getPaletteIntervalAddress<2>},
            {vidTimelineTargetId(VidTimelineTarget::PALETTE_INTERVAL_A), L"Palette", L"Iteration Interval A", TimelineParamKind::FLOAT, TimelineApplyCost::CHEAP, TimelineDirtyMask::PALETTE, 1.0f, 1.0e18f, &getPaletteInterval<3>, &setPaletteInterval<3>, nullptr, nullptr, &getPaletteIntervalAddress<3>},
            NESTED_VALUE(PALETTE_OFFSET_RATIO, L"Palette", L"Offset Ratio", FLOAT, PALETTE, 0.0f, 1.0f, palette, offsetRatio),
            NESTED_VALUE(PALETTE_CYCLE_BIAS, L"Palette", L"Cycle Bias", FLOAT, PALETTE, 0.10f, 4.00f, palette, cycleBias),
            NESTED_VALUE(PALETTE_CYCLE_CURVE, L"Palette", L"Cycle Curve", ENUM, PALETTE, 0.0f, 1.0f, palette, cycleCurve),
            NESTED_VALUE(PALETTE_ITERATION_COLORING, L"Palette", L"Iteration Coloring", ENUM, PALETTE, 0.0f, 6.0f, palette, iterationColoring),
            NESTED_VALUE(PALETTE_ANIMATION_SPEED, L"Palette", L"Animation Speed", FLOAT, PALETTE, -1000000.0f, 1000000.0f, palette, animationSpeed),
            NESTED_VALUE(PALETTE_FLOW_AMOUNT, L"Palette", L"Flow Amount", FLOAT, PALETTE, 0.0f, 1000000.0f, palette, animationFlowAmount),
            NESTED_VALUE(PALETTE_FLOW_SCALE, L"Palette", L"Flow Scale", FLOAT, PALETTE, 0.0f, 12.0f, palette, animationFlowScale),
            NESTED_VALUE(PALETTE_FLOW_SPEED, L"Palette", L"Flow Speed", FLOAT, PALETTE, -2.0f, 2.0f, palette, animationFlowSpeed),
            NESTED_VALUE(PALETTE_FLOW_SWIRL, L"Palette", L"Flow Swirl", FLOAT, PALETTE, -2.0f, 2.0f, palette, animationFlowSwirl),
            NESTED_VALUE(PALETTE_STATIC_COLOR_TOLERANCE, L"Palette", L"Static Color Tolerance", FLOAT, PALETTE, 0.0f, 1.0f, palette, staticColorTolerance),
            NESTED_COLOR(PALETTE_MANDELBROT_COLOR, L"Palette", L"Mandelbrot Color", PALETTE, palette, mandelbrotColor),

            NESTED_VALUE(STRIPE_TYPE, L"Stripe", L"Type", ENUM, STRIPE, 0.0f, 3.0f, stripe, stripeType),
            NESTED_VALUE(STRIPE_FIRST_INTERVAL, L"Stripe", L"First Interval", FLOAT, STRIPE, 0.0001f, 1000000.0f, stripe, firstInterval),
            NESTED_VALUE(STRIPE_SECOND_INTERVAL, L"Stripe", L"Second Interval", FLOAT, STRIPE, 0.0001f, 1000000.0f, stripe, secondInterval),
            NESTED_VALUE(STRIPE_OPACITY, L"Stripe", L"Opacity", FLOAT, STRIPE, 0.0f, 1.0f, stripe, opacity),
            NESTED_VALUE(STRIPE_OFFSET, L"Stripe", L"Offset", FLOAT, STRIPE, -1000000.0f, 1000000.0f, stripe, offset),
            NESTED_VALUE(STRIPE_ANIMATION_SPEED, L"Stripe", L"Animation Speed", FLOAT, STRIPE, -1000000.0f, 1000000.0f, stripe, animationSpeed),

            NESTED_VALUE(SLOPE_DEPTH, L"Slope", L"Depth", FLOAT, SLOPE, 0.0f, 10000.0f, slope, depth),
            NESTED_VALUE(SLOPE_REFLECTION_RATIO, L"Slope", L"Reflection Ratio", FLOAT, SLOPE, 0.0f, 1.0f, slope, reflectionRatio),
            NESTED_VALUE(SLOPE_OPACITY, L"Slope", L"Opacity", FLOAT, SLOPE, 0.0f, 1.0f, slope, opacity),
            NESTED_VALUE(SLOPE_ZENITH, L"Slope", L"Zenith", FLOAT, SLOPE, -360000.0f, 360000.0f, slope, zenith),
            NESTED_VALUE(SLOPE_AZIMUTH, L"Slope", L"Azimuth", FLOAT, SLOPE, -360000.0f, 360000.0f, slope, azimuth),
            NESTED_VALUE(SLOPE_SPECULAR_INTENSITY, L"Slope", L"Specular Intensity", FLOAT, SLOPE, 0.0f, 1.0f, slope, specularIntensity),
            NESTED_VALUE(SLOPE_SPECULAR_POWER, L"Slope", L"Specular Power", FLOAT, SLOPE, 1.0f, 100000.0f, slope, specularPower),
            NESTED_VALUE(SLOPE_RIM_INTENSITY, L"Slope", L"Rim Intensity", FLOAT, SLOPE, 0.0f, 1.0f, slope, rimIntensity),
            NESTED_VALUE(SLOPE_RIM_POWER, L"Slope", L"Rim Power", FLOAT, SLOPE, 1.0f, 64.0f, slope, rimPower),
            NESTED_VALUE(SLOPE_BRIGHTNESS, L"Slope", L"Brightness", FLOAT, SLOPE, 0.0001f, 1000000.0f, slope, brightness),
            NESTED_VALUE(SLOPE_GAMMA, L"Slope", L"Gamma", FLOAT, SLOPE, 0.0001f, 1000000.0f, slope, gamma),
            NESTED_COLOR(SLOPE_RIM_COLOR, L"Slope", L"Rim Color", SLOPE, slope, rimColor),
            NESTED_COLOR(SLOPE_SPECULAR_COLOR, L"Slope", L"Specular Color", SLOPE, slope, specularColor),
            NESTED_VALUE(SLOPE_AO_INTENSITY, L"Slope", L"AO Intensity", FLOAT, SLOPE, 0.0f, 1.0f, slope, aoIntensity),
            NESTED_VALUE(SLOPE_AMBIENT_INTENSITY, L"Slope", L"Ambient Intensity", FLOAT, SLOPE, 0.0f, 1.0f, slope, ambientIntensity),
            NESTED_COLOR(SLOPE_SKY_COLOR, L"Slope", L"Sky Color", SLOPE, slope, skyColor),
            NESTED_COLOR(SLOPE_GROUND_COLOR, L"Slope", L"Ground Color", SLOPE, slope, groundColor),
            NESTED_VALUE(SLOPE_SPECULAR_INDEPENDENT, L"Slope", L"Specular Independent", BOOL, SLOPE, 0.0f, 1.0f, slope, specularIndependent),
            NESTED_VALUE(SLOPE_SPECULAR_ZENITH, L"Slope", L"Specular Zenith", FLOAT, SLOPE, -360000.0f, 360000.0f, slope, specularZenith),
            NESTED_VALUE(SLOPE_SPECULAR_AZIMUTH, L"Slope", L"Specular Azimuth", FLOAT, SLOPE, -360000.0f, 360000.0f, slope, specularAzimuth),
            NESTED_VALUE(SLOPE_SPECULAR_ANISOTROPY, L"Slope", L"Specular Anisotropy", FLOAT, SLOPE, 0.0f, 1.0f, slope, specularAnisotropy),
            NESTED_VALUE(SLOPE_SPECULAR_ANISOTROPY_ANGLE, L"Slope", L"Anisotropy Angle", FLOAT, SLOPE, -360000.0f, 360000.0f, slope, specularAnisotropyAngle),
            NESTED_VALUE(SLOPE_MACRO_RELIEF, L"Slope", L"Macro Relief", FLOAT, SLOPE, 0.0f, 1.0f, slope, macroRelief),
            NESTED_VALUE(SLOPE_MACRO_RADIUS, L"Slope", L"Macro Radius", FLOAT, SLOPE, 1.0f, 12.0f, slope, macroRadius),
            NESTED_VALUE(SLOPE_RELIEF_RESPONSE, L"Slope", L"Relief Response", FLOAT, SLOPE, 0.0f, 1.0f, slope, reliefResponse),
            NESTED_VALUE(SLOPE_TERMINATOR_SOFTNESS, L"Slope", L"Terminator Softness", FLOAT, SLOPE, 0.0f, 1.0f, slope, terminatorSoftness),
            NESTED_VALUE(SLOPE_HIGHLIGHT_KNEE, L"Slope", L"Highlight Knee", FLOAT, SLOPE, 0.0f, 1.0f, slope, highlightKnee),
            NESTED_VALUE(SLOPE_LUMA_AMOUNT, L"Slope", L"Luma Amount", FLOAT, SLOPE, 0.0f, 1.0f, slope, lumaAmount),
            NESTED_VALUE(SLOPE_TINT_RESPONSE, L"Slope", L"Tint Response", FLOAT, SLOPE, 0.10f, 4.00f, slope, tintResponse),
            NESTED_VALUE(SLOPE_SHADOW_CHROMA, L"Slope", L"Shadow Chroma", FLOAT, SLOPE, 0.0f, 2.0f, slope, shadowChroma),
            NESTED_VALUE(SLOPE_FILL_INTENSITY, L"Slope", L"Fill Intensity", FLOAT, SLOPE, 0.0f, 1.0f, slope, fillIntensity),
            NESTED_VALUE(SLOPE_FILL_ZENITH, L"Slope", L"Fill Zenith", FLOAT, SLOPE, -360000.0f, 360000.0f, slope, fillZenith),
            NESTED_VALUE(SLOPE_FILL_AZIMUTH, L"Slope", L"Fill Direction", FLOAT, SLOPE, -360000.0f, 360000.0f, slope, fillAzimuth),

            NESTED_VALUE(COLOR_GAMMA, L"Color", L"Gamma", FLOAT, COLOR, 0.0001f, 1000000.0f, color, gamma),
            NESTED_VALUE(COLOR_EXPOSURE, L"Color", L"Exposure", FLOAT, COLOR, -1000000.0f, 1000000.0f, color, exposure),
            NESTED_VALUE(COLOR_HUE, L"Color", L"Hue", FLOAT, COLOR, -360000.0f, 360000.0f, color, hue),
            NESTED_VALUE(COLOR_SATURATION, L"Color", L"Saturation", FLOAT, COLOR, -1000000.0f, 1000000.0f, color, saturation),
            NESTED_VALUE(COLOR_BRIGHTNESS, L"Color", L"Brightness", FLOAT, COLOR, -1000000.0f, 1000000.0f, color, brightness),
            NESTED_VALUE(COLOR_CONTRAST, L"Color", L"Contrast", FLOAT, COLOR, -1.0f, 1.0f, color, contrast),

            NESTED_VALUE(FOG_RADIUS, L"Fog", L"Radius", FLOAT, FOG, 0.0f, 1.0f, fog, radius),
            NESTED_VALUE(FOG_OPACITY, L"Fog", L"Opacity", FLOAT, FOG, 0.0f, 1.0f, fog, opacity),
            NESTED_VALUE(FOG_CENTER_START, L"Fog", L"Center Start", FLOAT, FOG, 0.0f, 1.0f, fog, centerStart),
            NESTED_VALUE(FOG_CENTER_INVERT, L"Fog", L"Center Invert", BOOL, FOG, 0.0f, 1.0f, fog, centerInvert),
            NESTED_VALUE(FOG_RIM_MASK, L"Fog", L"Rim Mask", FLOAT, FOG, 0.0f, 1.0f, fog, rimMask),
            NESTED_VALUE(FOG_RIM_MASK_BOOST, L"Fog", L"Rim Mask Boost", FLOAT, FOG, 0.0001f, 1000000.0f, fog, rimMaskBoost),
            NESTED_VALUE(FOG_RIM_BLUR, L"Fog", L"Rim Blur", FLOAT, FOG, 0.0f, 1000000.0f, fog, rimBlur),
            NESTED_VALUE(FOG_FOCUS_AMOUNT, L"Fog", L"Focus Amount", FLOAT, FOG, 0.0f, 1.0f, fog, focusAmount),
            NESTED_VALUE(FOG_FOCUS_RATIO, L"Fog", L"Focus Ratio", FLOAT, FOG, 0.0f, 1.0f, fog, focusRatio),
            NESTED_VALUE(FOG_FOCUS_RANGE, L"Fog", L"Focus Range", FLOAT, FOG, 0.01f, 1.0f, fog, focusRange),
            NESTED_VALUE(FOG_FOCUS_FALLOFF, L"Fog", L"Focus Falloff", FLOAT, FOG, 0.10f, 4.00f, fog, focusFalloff),
            NESTED_VALUE(FOG_FOCUS_BLUR, L"Fog", L"Focus Blur", FLOAT, FOG, 0.0f, 1000000.0f, fog, focusBlur),

            NESTED_VALUE(BLOOM_THRESHOLD, L"Bloom", L"Threshold", FLOAT, BLOOM, 0.0f, 1.0f, bloom, threshold),
            NESTED_VALUE(BLOOM_RADIUS, L"Bloom", L"Radius", FLOAT, BLOOM, 0.0f, 1.0f, bloom, radius),
            NESTED_VALUE(BLOOM_SOFTNESS, L"Bloom", L"Softness", FLOAT, BLOOM, 0.0f, 1.0f, bloom, softness),
            NESTED_VALUE(BLOOM_INTENSITY, L"Bloom", L"Intensity", FLOAT, BLOOM, 0.0f, 1000000.0f, bloom, intensity),

            TEXTURE_LAYER(1),
            TEXTURE_LAYER(2),
            TEXTURE_LAYER(3),
            TEXTURE_LAYER(4),
            PATTERN_LAYER(1),
            PATTERN_LAYER(2),
            PATTERN_LAYER(3),
            PATTERN_LAYER(4),

            NESTED_VALUE(WARP_ENABLED, L"Warp", L"Enabled", BOOL, WARP, 0.0f, 1.0f, warp, enabled),
            NESTED_VALUE(WARP_SOURCE, L"Warp", L"Source", ENUM, WARP, 0.0f, 4.0f, warp, source),
            NESTED_VALUE(WARP_UV_MODE, L"Warp", L"UV Mode", ENUM, WARP, 0.0f, 3.0f, warp, uvMode),
            NESTED_VALUE(WARP_AMOUNT, L"Warp", L"Amount", FLOAT, WARP, 0.0f, 2.0f, warp, amount),
            NESTED_VALUE(WARP_OCTAVES, L"Warp", L"Octaves", FLOAT, WARP, 1.0f, 6.0f, warp, octaves),
            NESTED_VALUE(WARP_SCALE_U, L"Warp", L"Scale U", FLOAT, WARP, 0.0f, 200.0f, warp, scaleU),
            NESTED_VALUE(WARP_SCALE_V, L"Warp", L"Scale V", FLOAT, WARP, 0.0f, 200.0f, warp, scaleV),
            NESTED_VALUE(WARP_SCROLL_U, L"Warp", L"Scroll U", FLOAT, WARP, -2.0f, 2.0f, warp, scrollU),
            NESTED_VALUE(WARP_SCROLL_V, L"Warp", L"Scroll V", FLOAT, WARP, -2.0f, 2.0f, warp, scrollV),
            NESTED_VALUE(WARP_PALETTE_FOLLOW, L"Warp", L"Palette Follow", FLOAT, WARP, -2.0f, 2.0f, warp, paletteFollow),
            NESTED_VALUE(WARP_PERIOD, L"Warp", L"Period Iterations", FLOAT, WARP, 0.0f, 1000000000.0f, warp, periodIterations),
        });

#undef PATTERN_LAYER
#undef TEXTURE_LAYER
#undef PATTERN_COLOR
#undef PATTERN_VALUE
#undef TEXTURE_VALUE
#undef WIDEN
#undef WIDEN_INNER
#undef NESTED_COLOR
#undef NESTED_VALUE
    }

    std::span<const TimelineParamDesc> TimelineParams::all() {
        return PARAMS;
    }

    const TimelineParamDesc *TimelineParams::find(const uint16_t id) {
        for (const auto &param: PARAMS) {
            if (param.id == id) {
                return &param;
            }
        }
        return nullptr;
    }

    bool TimelineParams::movesOverStaticImage(const uint16_t id) {
        const TimelineParamDesc *param = find(id);
        if (param == nullptr) {
            return true;
        }
        // The palette, the stripe, the textures, the patterns and the warp are all drawn by the pass
        // that reads the iteration buffer, which a PNG source skips, and the slope reads the relief
        // that same buffer carries. See VideoRenderSceneRenderer::cmdRender.
        switch (param->dirty) {
            using enum TimelineDirtyMask;
            case PALETTE:
            case STRIPE:
            case SLOPE:
            case TEXTURE:
            case PATTERN:
            case WARP:
                return false;
            default:
                break;
        }
        // The rim band reads the relief and the focus band reads the iteration count, so those two
        // fog controls and the ones that only shape them stand still over a PNG as well.
        switch (static_cast<VidTimelineTarget>(id)) {
            using enum VidTimelineTarget;
            case FOG_RIM_MASK:
            case FOG_RIM_MASK_BOOST:
            case FOG_RIM_BLUR:
            case FOG_FOCUS_AMOUNT:
            case FOG_FOCUS_RATIO:
            case FOG_FOCUS_RANGE:
            case FOG_FOCUS_FALLOFF:
            case FOG_FOCUS_BLUR:
                return false;
            default:
                return true;
        }
    }
}
