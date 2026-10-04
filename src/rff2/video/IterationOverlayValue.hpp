//
// Modified by GPT-6 on 2026-09-26
//

#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>

namespace merutilm::rff2 {
    inline std::optional<uint64_t> iterationOverlayValue(std::optional<uint64_t> normal,
                                                         std::optional<uint64_t> zoomed,
                                                         float depth, bool interpolate) {
        if (depth < 1) return normal;
        if (!normal || !zoomed) return {};
        if (!interpolate) return std::max(*normal, *zoomed);
        const long double fraction = depth - std::floor(depth);
        const uint64_t difference = *normal >= *zoomed ? *normal - *zoomed : *zoomed - *normal;
        const auto step = static_cast<uint64_t>(std::round(static_cast<long double>(difference) * fraction));
        return *normal >= *zoomed ? *zoomed + step : *zoomed - step;
    }
}
