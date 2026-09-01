//
// Created by Opus 5 on 2026-08-20.
// Modified by Opus 5 on 2026-08-22.
//

#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>

#include "../attr/ShdPaletteAttribute.h"

namespace merutilm::rff2 {
    // Entries a band is drawn out to before the lines are laid in, so the thinnest line the
    // settings allow still lands on one of them. Bounded because the whole array is uploaded.
    inline constexpr uint64_t BAND_LINE_MIN_ENTRIES = 256;
    inline constexpr uint64_t BAND_LINE_MAX_ENTRIES = 4096;
    inline constexpr uint64_t BAND_LINE_MAX_TOTAL = 1u << 20;

    // Lays the band lines over the copy that is uploaded, so the palette keeps the colors as they
    // were edited and the lines can be moved or lifted without regenerating them. Shared by both
    // palette uploads, so the preview and a video export draw the same lines.
    inline std::vector<glm::vec4> applyBandLines(std::vector<glm::vec4> colors, const ShdPaletteAttribute &palette) {
        const auto count = static_cast<uint64_t>(palette.bandLineCount);
        if (!palette.bandLineEnabled || colors.empty() || count == 0 || palette.bandLineWidth <= 0.0f) {
            return colors;
        }

        // A hard line only has to land on an entry; a feathered one is a ramp, and a ramp drawn
        // over a handful of entries comes out as steps, so softness asks for far more of them.
        const float span = palette.bandLineSoftness > 0.0f ? 64.0f : 4.0f;
        const auto perBand = std::clamp(static_cast<uint64_t>(std::ceil(span / palette.bandLineWidth)),
                                        BAND_LINE_MIN_ENTRIES, BAND_LINE_MAX_ENTRIES);
        const uint64_t target = std::min(count * perBand, BAND_LINE_MAX_TOTAL);

        // A palette too coarse to hold the line is interpolated up first, in the space the shader
        // would have blended it in, so the colors between the lines are the same ones.
        if (static_cast<uint64_t>(colors.size()) < target) {
            const auto src = static_cast<double>(colors.size());
            std::vector<glm::vec4> fine;
            fine.reserve(target);
            for (uint64_t i = 0; i < target; ++i) {
                const double pos = static_cast<double>(i) * src / static_cast<double>(target);
                const auto i0 = static_cast<size_t>(pos) % colors.size();
                const size_t i1 = (i0 + 1) % colors.size();
                fine.push_back(blendPaletteColors(colors[i0], colors[i1],
                                                  static_cast<float>(pos - std::floor(pos)),
                                                  palette.colorInterpolation));
            }
            colors = std::move(fine);
        }

        const auto size = static_cast<float>(colors.size());
        for (size_t i = 0; i < colors.size(); ++i) {
            const float coverage = palette.bandLineCoverage(static_cast<float>(i) / size);
            if (coverage > 0.0f) {
                colors[i] = glm::mix(colors[i], palette.bandLineColor, coverage);
            }
        }
        return colors;
    }
}
