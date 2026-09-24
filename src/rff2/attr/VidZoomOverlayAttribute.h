//
// Modified by GPT-6 on 2026-09-18, 2026-09-20, 2026-09-23
//

#pragma once
#include <cstdint>
#include <string>
#include <glm/glm.hpp>

namespace merutilm::rff2 {
    struct VidZoomOverlayAttribute {
        uint32_t decimalPlaces = 6;
        bool visible = true;
        bool custom = false;
        uint32_t anchor = 0;
        float x = 0.02f;
        float y = 0.02f;
        float size = 0.03f;
        std::string family = "Segoe UI";
        uint32_t style = 0;
        glm::vec4 color{1, 1, 1, 1};
        bool outline = true;
        float outlineWidth = 0.06f;
        glm::vec4 outlineColor{0, 0, 0, 1};
        bool shadow = false;
        float shadowX = 0.08f;
        float shadowY = 0.08f;
        glm::vec4 shadowColor{0, 0, 0, 1};
    };
}
