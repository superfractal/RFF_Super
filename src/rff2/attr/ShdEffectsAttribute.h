//
// Modified by GPT-6 on 2026-09-11, 2026-09-23
// Modified by Opus 5.5 on 2026-10-20, 2026-10-21
//

#pragma once
#include <array>
#include <cstdint>
#include <glm/glm.hpp>

namespace merutilm::rff2 {
    enum class ShdEffectType {
        RAIN = 0,
        FLAME = 1,
        EMBERS = 2,
        MIST = 3,
        HEAT = 4,
        RIPPLES = 5,
        FLOW = 6,
        AURORA = 7
    };
    enum class ShdEffectBlend {
        NORMAL = 0,
        ADD = 1,
        SCREEN = 2,
        MULTIPLY = 3
    };
    enum class ShdEffectMask {
        ALL = 0,
        EXTERIOR = 1,
        INTERIOR = 2,
        BANDS = 3,
        EDGE = 4
    };
    enum class ShdRainShape {
        STREAKS = 0,
        DROPS = 1
    };
    // Each style list starts with CLASSIC, the look the type had before styles existed.
    enum class ShdFlameStyle {
        CLASSIC = 0,
        TURBULENT = 1,
        BURNING_EDGE = 2
    };
    enum class ShdEmbersStyle {
        CLASSIC = 0,
        RISING = 1,
        BURST = 2
    };
    enum class ShdMistStyle {
        CLASSIC = 0,
        DRIFTING = 1,
        POOLED = 2
    };
    enum class ShdRipplesStyle {
        CLASSIC = 0,
        INTERFERENCE = 1,
        CAUSTICS = 2
    };
    enum class ShdFlowStyle {
        CLASSIC = 0,
        CHASE = 1
    };
    enum class ShdAuroraStyle {
        CLASSIC = 0,
        CURTAIN = 1,
        RIBBONS = 2
    };
    // One Effect Type menu entry: the type plus its Rain Shape or style, packed as type + 8 * shape-or-style.
    enum class ShdEffectVariant {
        RAIN_STREAKS = 0,
        RAIN_DROPS = 8,
        FLAME = 1,
        FLAME_TURBULENT = 9,
        FLAME_BURNING_EDGE = 17,
        EMBERS = 2,
        EMBERS_RISING = 10,
        EMBERS_BURST = 18,
        MIST = 3,
        MIST_DRIFTING = 11,
        MIST_POOLED = 19,
        RIPPLES = 5,
        RIPPLES_INTERFERENCE = 13,
        RIPPLES_CAUSTICS = 21,
        FLOW = 6,
        FLOW_CHASE = 14,
        AURORA = 7,
        AURORA_CURTAIN = 15,
        AURORA_RIBBONS = 23
    };
    constexpr uint32_t EFFECT_LAYER_COUNT = 4;
    struct ShdEffectAttribute {
        bool enabled = false;
        ShdEffectType type = ShdEffectType::RAIN;
        ShdEffectBlend blend = ShdEffectBlend::SCREEN;
        ShdEffectMask mask = ShdEffectMask::ALL;
        float opacity = 0.65f;
        float scale = 1.0f;
        float speed = 1.0f;
        float evolution = 0.4f;
        float density = 0.45f;
        float length = 0.65f;
        float width = 1.0f;
        float direction = 12.0f;
        float distortion = 0.3f;
        float glow = 0.25f;
        float seed = 1.0f;
        float period = 80.0f;
        glm::vec4 color = {0.64f, 0.79f, 1.0f, 1.0f};
        glm::vec4 secondary = {1.0f, 0.25f, 0.025f, 1.0f};
        ShdRainShape rainShape = ShdRainShape::DROPS;
        float dropSize = 1.0f;
        bool syncColorAnimation = false;
        ShdFlameStyle flameStyle = ShdFlameStyle::CLASSIC;
        ShdEmbersStyle embersStyle = ShdEmbersStyle::CLASSIC;
        ShdMistStyle mistStyle = ShdMistStyle::CLASSIC;
        ShdRipplesStyle ripplesStyle = ShdRipplesStyle::CLASSIC;
        ShdFlowStyle flowStyle = ShdFlowStyle::CLASSIC;
        ShdAuroraStyle auroraStyle = ShdAuroraStyle::CLASSIC;

        // The style index of this layer's own type, as the shader reads it; 0 for Rain.
        [[nodiscard]] uint32_t styleIndex() const {
            switch (type) {
                case ShdEffectType::FLAME: return uint32_t(flameStyle);
                case ShdEffectType::EMBERS: return uint32_t(embersStyle);
                case ShdEffectType::MIST: return uint32_t(mistStyle);
                case ShdEffectType::RIPPLES: return uint32_t(ripplesStyle);
                case ShdEffectType::FLOW: return uint32_t(flowStyle);
                case ShdEffectType::AURORA: return uint32_t(auroraStyle);
                default: return 0;
            }
        }
    };
    using ShdEffectsAttribute = std::array<ShdEffectAttribute, EFFECT_LAYER_COUNT>;

    [[nodiscard]] inline ShdEffectVariant effectVariantOf(const ShdEffectAttribute &effect) {
        const uint32_t detail = effect.type == ShdEffectType::RAIN ? uint32_t(effect.rainShape) : effect.styleIndex();
        return static_cast<ShdEffectVariant>(uint32_t(effect.type) + 8 * detail);
    }

    // Sets the type and the one shape or style the entry names; the other types' styles are left as they were.
    inline void applyEffectVariant(ShdEffectAttribute &effect, const ShdEffectVariant variant) {
        const uint32_t value = uint32_t(variant);
        effect.type = static_cast<ShdEffectType>(value % 8);
        const uint32_t detail = value / 8;
        switch (effect.type) {
            case ShdEffectType::RAIN: effect.rainShape = static_cast<ShdRainShape>(detail); break;
            case ShdEffectType::FLAME: effect.flameStyle = static_cast<ShdFlameStyle>(detail); break;
            case ShdEffectType::EMBERS: effect.embersStyle = static_cast<ShdEmbersStyle>(detail); break;
            case ShdEffectType::MIST: effect.mistStyle = static_cast<ShdMistStyle>(detail); break;
            case ShdEffectType::RIPPLES: effect.ripplesStyle = static_cast<ShdRipplesStyle>(detail); break;
            case ShdEffectType::FLOW: effect.flowStyle = static_cast<ShdFlowStyle>(detail); break;
            case ShdEffectType::AURORA: effect.auroraStyle = static_cast<ShdAuroraStyle>(detail); break;
            default: break;
        }
    }
}
