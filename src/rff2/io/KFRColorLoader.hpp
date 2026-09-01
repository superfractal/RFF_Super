//
// Created by Merutilm on 2025-07-16.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21.
// Modified by Opus 5 on 2026-08-20
//

#pragma once
#include <string>
#include <glm/vec4.hpp>

#include "../attr/ShdPaletteAttribute.h"

namespace merutilm::rff2 {
    struct KFRColorLoader {
        KFRColorLoader() = delete;

        static std::vector<glm::vec4> loadPaletteSettings();
        static std::vector<glm::vec4> parseColorString(const std::wstring& colorStr);
        static std::vector<glm::vec4> generateRandomPalette(uint32_t colorCount);
    };
}
