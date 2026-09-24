//
// Modified by GPT-6 on 2026-09-11, 2026-09-23
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
    };
    using ShdEffectsAttribute = std::array<ShdEffectAttribute, EFFECT_LAYER_COUNT>;
}
