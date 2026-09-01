//
// Created by Opus 5 on 2026-09-01
//

#include "PreferencesIO.h"

#include <filesystem>
#include <format>
#include <fstream>
#include <system_error>

#include "../constants/Constants.hpp"
#include "../ui/IOUtilities.h"
#include "../ui/SettingsTheme.hpp"
#include "../ui/Utilities.h"

namespace merutilm::rff2 {
    namespace {
        std::filesystem::path preferencesFile() {
            return Utilities::getDefaultPath() /
                   std::format(L"preferences.{}", Constants::Extension::PREFERENCES);
        }
    }

    void PreferencesIO::load() {
        std::ifstream in(preferencesFile(), std::ios::in | std::ios::binary);
        if (!in.is_open()) {
            return;
        }
        uint32_t magic = 0;
        uint32_t version = 0;
        IOUtilities::readAndDecode(in, &magic);
        IOUtilities::readAndDecode(in, &version);
        if (in.fail() || magic != MAGIC || version > VERSION) {
            return;
        }
        // Started from what is already live, so a file ending early leaves that setting as it is.
        bool dark = darkSettingsMode();
        bool timelineLight = timelineLightMode();
        IOUtilities::readAndDecode(in, &dark);
        IOUtilities::readAndDecode(in, &timelineLight);
        if (in.fail()) {
            return;
        }
        darkSettingsModeFlag() = dark;
        timelineLightModeFlag().store(timelineLight, std::memory_order_relaxed);
    }

    void PreferencesIO::save() {
        const std::filesystem::path path = preferencesFile();
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            return;
        }
        IOUtilities::encodeAndWrite(out, MAGIC);
        IOUtilities::encodeAndWrite(out, VERSION);
        IOUtilities::encodeAndWrite(out, darkSettingsMode());
        IOUtilities::encodeAndWrite(out, timelineLightMode());
    }
}
