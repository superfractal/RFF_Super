//
// Created by Merutilm on 2025-07-16.
// Modified by AI; earlier exact modification date unavailable.
// Modified by Opus 5 on 2026-08-20
// Modified by GPT-5 on 2026-08-21
// Modified by GPT-6 on 2026-09-23
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
        
        auto channels = Utilities::split(line, ',');
        if (channels.empty()) {
            return {};
        }
        
        std::vector<float> channelValues;
        channelValues.reserve(channels.size());
        for (std::wstring &channel : channels) {
            std::erase(channel, ' ');
            channelValues.push_back(std::stof(channel) / 255.0f);
        }
        
        std::vector<glm::vec4> colors;
        colors.reserve(channelValues.size() / 3);
        for (size_t offset = 0; offset + 2 < channelValues.size(); offset += 3) {
            const float blue = channelValues[offset];
            const float green = channelValues[offset + 1];
            const float red = channelValues[offset + 2];
            colors.emplace_back(red, green, blue, 1.0f);
        }
        return colors;
    }

    std::vector<glm::vec4> KFRColorLoader::generateRandomPalette(uint32_t colorCount) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(0, 255);
        
        std::vector<glm::vec4> colors;
        colors.reserve(colorCount);
        for (uint32_t i = 0; i < colorCount; ++i) {
            const float blue = static_cast<float>(dist(gen)) / 255.0f;
            const float green = static_cast<float>(dist(gen)) / 255.0f;
            const float red = static_cast<float>(dist(gen)) / 255.0f;
            colors.emplace_back(red, green, blue, 1.0f);
        }
        return colors;
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
