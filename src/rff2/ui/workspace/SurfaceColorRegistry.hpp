//
// Modified by GPT-6 on 2026-09-13, 2026-09-18, 2026-09-19, 2026-09-23, 2026-09-24
//

#pragma once
#include "SurfaceParameterRegistry.hpp"
#include <cmath>

namespace merutilm::rff2::workspace {
    struct SurfaceColor {
        std::string_view id;
        std::wstring_view label;
        glm::vec4 ShdSlopeAttribute::*member;
        Category category;

        bool valid(const glm::vec4 &color) const {
            for (int channel = 0; channel < 4; ++channel) {
                if (!std::isfinite(color[channel]) || color[channel] < 0 || color[channel] > 1) {
                    return false;
                }
            }
            return true;
        }
    };

    inline const std::vector<SurfaceColor> &surfaceColors() {
        static const std::vector<SurfaceColor> colors = {
            {"surface.styleColor", L"Surface Color",
             &ShdSlopeAttribute::styleColor, Category::COLOR},
            {"surface.styleHighlightColor", L"Reflection Color",
             &ShdSlopeAttribute::styleHighlightColor, Category::COLOR},
            {"surface.styleBackgroundColor", L"Background Color",
             &ShdSlopeAttribute::styleBackgroundColor, Category::COLOR},
            {"surface.styleRimColor", L"Detail Rim Color",
             &ShdSlopeAttribute::styleRimColor, Category::COLOR},
            {"surface.seaBodyColor", L"Body Color",
             &ShdSlopeAttribute::seaBodyColor, Category::EMISSION},
            {"surface.seaGlowColor", L"Emission Color A",
             &ShdSlopeAttribute::seaGlowColor, Category::EMISSION},
            {"surface.seaAccentColor", L"Emission Color B",
             &ShdSlopeAttribute::seaAccentColor, Category::EMISSION},
            {"surface.printInk", L"Ink",
             &ShdSlopeAttribute::printInk, Category::PRINT},
            {"surface.printIndigo", L"Indigo",
             &ShdSlopeAttribute::printIndigo, Category::PRINT},
            {"surface.printAsagi", L"Asagi",
             &ShdSlopeAttribute::printAsagi, Category::PRINT},
            {"surface.printBlue", L"Pale Blue",
             &ShdSlopeAttribute::printBlue, Category::PRINT},
            {"surface.printFoam", L"Foam",
             &ShdSlopeAttribute::printFoam, Category::PRINT},
            {"surface.printPaper", L"Paper",
             &ShdSlopeAttribute::printPaper, Category::PRINT}
        };
        return colors;
    }
}
