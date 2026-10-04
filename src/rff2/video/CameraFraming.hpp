//
// Modified by GPT-6 on 2026-10-02
//

#pragma once
#include <cmath>
#include <cstdint>
#include <numbers>
#include <optional>

namespace merutilm::rff2 {
    inline std::optional<float> planarCameraFov(uint32_t width, uint32_t height) {
        if (!width || !height) return std::nullopt;
        const double fov = 2.0 * std::atan(static_cast<double>(width) / height) * 180.0 / std::numbers::pi;
        if (fov < 1.0 || fov > 179.0) return std::nullopt;
        return static_cast<float>(fov);
    }
}
