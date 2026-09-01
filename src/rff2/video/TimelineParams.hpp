// Modified by GPT-5 on 2026-08-18
// Modified by Opus 5 on 2026-08-26, 2026-09-01

#pragma once

#include <cstdint>
#include <span>

#include <glm/glm.hpp>

#include "../attr/ShaderAttribute.h"

namespace merutilm::rff2 {
    enum class TimelineParamKind : uint8_t {
        FLOAT,
        COLOR,
        BOOL,
        ENUM
    };

    enum class TimelineApplyCost : uint8_t {
        CHEAP,
        PALETTE,
        TEXTURE
    };

    enum class TimelineDirtyMask : uint32_t {
        NONE = 0,
        PALETTE = 1u << 0,
        STRIPE = 1u << 1,
        SLOPE = 1u << 2,
        COLOR = 1u << 3,
        FOG = 1u << 4,
        BLOOM = 1u << 5,
        TEXTURE = 1u << 6,
        PATTERN = 1u << 7,
        WARP = 1u << 8,
        ALL = (1u << 9) - 1u
    };

    constexpr TimelineDirtyMask operator|(const TimelineDirtyMask a, const TimelineDirtyMask b) {
        return static_cast<TimelineDirtyMask>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    constexpr TimelineDirtyMask &operator|=(TimelineDirtyMask &a, const TimelineDirtyMask b) {
        a = a | b;
        return a;
    }

    constexpr bool hasTimelineDirty(const TimelineDirtyMask mask, const TimelineDirtyMask bit) {
        return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(bit)) != 0;
    }

    struct TimelineParamDesc {
        using GetValue = float (*)(const ShaderAttribute &);
        using SetValue = void (*)(ShaderAttribute &, float);
        using GetColor = glm::vec4 (*)(const ShaderAttribute &);
        using SetColor = void (*)(ShaderAttribute &, const glm::vec4 &);
        // Where the parameter itself sits inside a shader attribute. A settings panel binds its rows
        // to those addresses, so this is what tells a row driving this parameter from one that does
        // not - see SettingsWindow::disableRowsInObjectExcept.
        using GetAddress = const void *(*)(const ShaderAttribute &);

        uint16_t id;
        const wchar_t *group;
        const wchar_t *name;
        TimelineParamKind kind;
        TimelineApplyCost cost;
        TimelineDirtyMask dirty;
        float minValue;
        float maxValue;
        GetValue getValue;
        SetValue setValue;
        GetColor getColor;
        SetColor setColor;
        GetAddress address;
    };

    class TimelineParams final {
    public:
        TimelineParams() = delete;

        static std::span<const TimelineParamDesc> all();

        static const TimelineParamDesc *find(uint16_t id);

        // Whether a PNG source can move this parameter at all: a finished picture never runs the pass
        // that reads the iteration buffer, so everything that pass draws is dead over one.
        static bool movesOverStaticImage(uint16_t id);
    };
}
