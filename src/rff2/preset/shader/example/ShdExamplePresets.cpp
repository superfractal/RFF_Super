//
// Created by Opus 5 on 2026-08-31.
// Modified by Opus 5 on 2026-09-01
//

#include "ShdExamplePresets.h"

#include <algorithm>
#include <fstream>
#include <utility>

#include "../../../../vulkan_helper/core/logger.hpp"
#include "../../../io/ConfigIO.h"
#include "../../../io/ShaderPresetIO.h"
#include "../../../ui/IOUtilities.h"
#include "../../../ui/Utilities.h"

namespace merutilm::rff2 {
    namespace {
        // The magic word a file opens with, which is what says whether it carries a shader at all.
        // Zero for a file too short to hold one, which is every file this must pass over.
        uint32_t readMagic(const std::filesystem::path &path) {
            std::ifstream in(path, std::ios::in | std::ios::binary);
            if (!in.is_open()) {
                return 0;
            }
            uint32_t magic = 0;
            IOUtilities::readAndDecode(in, &magic);
            return in.fail() ? 0 : magic;
        }
    }

    ShdExamplePresets::FromFile::FromFile(std::filesystem::path path, std::string name, const bool wholeConfig)
        : path(std::move(path)), name(std::move(name)), wholeConfig(wholeConfig) {
    }

    std::string ShdExamplePresets::FromFile::getName() const {
        return name;
    }

    ShaderAttribute ShdExamplePresets::FromFile::genShader() const {
        ShaderAttribute shader = {};
        if (wholeConfig ? ConfigIO::loadShader(path, shader) : ShaderPresetIO::load(path, shader)) {
            return shader;
        }
        vkh::logger::w_log(L"ERROR : Cannot read the example shader");
        return {};
    }

    std::vector<ShdExamplePresets::FromFile> ShdExamplePresets::collect() {
        std::vector<FromFile> presets = {};
        std::error_code error;
        const std::filesystem::directory_iterator folder(Utilities::getDefaultPath() / EXAMPLE_FOLDER_NAME, error);
        if (error) {
            return presets;
        }
        for (const auto &entry: folder) {
            if (!entry.is_regular_file(error) || error) {
                continue;
            }
            const uint32_t magic = readMagic(entry.path());
            if (magic != ConfigIO::MAGIC && magic != ShaderPresetIO::MAGIC) {
                continue;
            }
            presets.emplace_back(entry.path(), entry.path().stem().string(), magic == ConfigIO::MAGIC);
        }
        std::ranges::sort(presets, [](const FromFile &a, const FromFile &b) { return a.name < b.name; });
        return presets;
    }
}
