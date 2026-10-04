//
// Modified by GPT-6 on 2026-09-25
// Modified by Opus 5.5 on 2026-10-04
//

#pragma once
#include <cstdint>

namespace merutilm::rff2 {
    struct MapLimits {
        static constexpr uint64_t MAX_PIXELS = 200'000'000;
        static constexpr bool valid(const uint32_t width, const uint32_t height) {
            return width > 0 && height > 0 && static_cast<uint64_t>(width) * height <= MAX_PIXELS;
        }
    };
}
