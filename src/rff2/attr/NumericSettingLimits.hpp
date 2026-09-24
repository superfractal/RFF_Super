// Modified by GPT-6 on 2026-09-24
#pragma once

#include <bit>
#include <cstdint>
#include <limits>

namespace merutilm::rff2::NumericSettingLimits {
    struct Range {
        float minimum;
        float maximum;

        static constexpr uint32_t orderedBits(float value) {
            auto bits = std::bit_cast<uint32_t>(value);
            if ((bits & 0x7fffffffU) == 0) bits = 0;
            return (bits & 0x80000000U) ? ~bits : (bits ^ 0x80000000U);
        }

        constexpr bool operator()(const float &value) const {
            return orderedBits(value) >= orderedBits(minimum) && orderedBits(value) <= orderedBits(maximum);
        }
    };

    inline constexpr Range gamma{0.01f, 10.f};
    inline constexpr Range colorExposure{-1.f, 0.999f};
    inline constexpr Range colorHue{-1.f, 1.f};
    inline constexpr Range timelineHue{-1000.f, 1000.f};
    inline constexpr Range saturation{-1.f, 10.f};
    inline constexpr Range colorBrightness{-1.f, 1.f};
    inline constexpr Range contrast{-1.f, 0.999f};
    inline constexpr Range slopeBrightness{0.f, 100.f};
    inline constexpr Range bloomIntensity{0.f, 100.f};
    inline constexpr Range rimMaskBoost{1.f, 100.f};
    inline constexpr Range blur{0.f, 256.f};
    inline constexpr Range logZoom{0.f, 16777216.f};
    inline constexpr Range estimatedKeyframes{1.f, 100000.f};
    inline constexpr Range hdrExposure{-20.f, 20.f};
    inline constexpr Range hdrHeadroom{1.f, 64.f};

    inline uint64_t automaticIterationLimit(uint64_t period, uint16_t multiplier) {
        if (period == 0) period = 1;
        if (multiplier == 0) multiplier = 1;
        const auto maximum = std::numeric_limits<uint64_t>::max();
        return period > maximum / multiplier ? maximum : period * multiplier;
    }

    inline constexpr float minimumFps = 1.f;
    inline constexpr float maximumFps = 1000.f;
    inline constexpr float minimumOverZoom = 0.f;
    inline constexpr float maximumOverZoom = 8.f;

    // Integer bit comparisons reject non-finite inputs even under fast-math.
    inline bool acceptsNonnegativeRange(float value, float minimum, float maximum) {
        auto bits = std::bit_cast<uint32_t>(value);
        if ((bits & 0x7fffffffU) == 0) bits = 0;
        return bits >= std::bit_cast<uint32_t>(minimum) && bits <= std::bit_cast<uint32_t>(maximum);
    }

    inline bool acceptsFps(const float &value) {
        return acceptsNonnegativeRange(value, minimumFps, maximumFps);
    }

    inline bool acceptsOverZoom(const float &value) {
        return acceptsNonnegativeRange(value, minimumOverZoom, maximumOverZoom);
    }
}
