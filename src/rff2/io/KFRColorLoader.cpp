//
// Created by Merutilm on 2025-07-16.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21.
// Modified by Opus 5 on 2026-08-20
//

#include "KFRColorLoader.hpp"
#include <fstream>
#include <random>

#include "../ui/IOUtilities.h"

namespace merutilm::rff2 {
    std::vector<glm::vec4> KFRColorLoader::parseColorString(const std::wstring& colorStr) {
        std::wstring line = colorStr;
        const std::wstring token = L"Colors: ";
        
        // Remove "Colors: " prefix if present
        if (line.starts_with(token)) {
            line = line.substr(token.length());
        }
        
        auto split = Utilities::split(line, ',');
        if (split.empty()) {
            return {};
        }
        
        auto result = std::vector<float>(split.size());
        std::ranges::transform(split, result.begin(), [](std::wstring str) {
            std::erase(str, ' ');
            return std::stof(str) / 255.0f;
        });
        
        auto out = std::vector<glm::vec4>();
        for (uint32_t i = 0; i + 2 < result.size(); i += 3) {
            out.emplace_back(result[i + 2], result[i + 1], result[i + 0], 1);
        }
        return out;
    }

    std::vector<glm::vec4> KFRColorLoader::generateRandomPalette(uint32_t colorCount) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(0, 255);
        
        std::wstring colorStr;
        for (uint32_t i = 0; i < colorCount * 3; ++i) {
            if (i > 0) colorStr += L",";
            colorStr += std::to_wstring(dist(gen));
        }
        
        return parseColorString(colorStr);
    }
    std::vector<glm::vec4> KFRColorLoader::loadPaletteSettings() {
        const auto pFile = IOUtilities::ioFileDialog(L"Open KFR Palette", Constants::Extension::DESC_KFR,
                                                     IOUtilities::OPEN_FILE, Constants::Extension::KFR);
        if (pFile == nullptr) {
            return {};
        }
        const auto &file = *pFile;
        std::wifstream stream(file, std::ios::in);
        if (!stream.is_open()) {
            MessageBox(nullptr, "Can't open KFR Palette", "Error", MB_OK | MB_ICONERROR);
            return {};
        }
        std::wstring line;
        const std::wstring token = L"Colors: ";
        while (getline(stream, line)) {
            if (!line.starts_with(token)) {
                continue;
            }
            return parseColorString(line);
        }
        return {};
    }
}
