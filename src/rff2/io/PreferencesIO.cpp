//
// Created by Opus 5 on 2026-09-01
// Modified by GPT-6 on 2026-09-15, 2026-09-18, 2026-09-19, 2026-09-21, 2026-09-23
//

#include "PreferencesIO.h"
#include "../ui/UiLanguage.hpp"

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

        bool hasMoreBytes(std::ifstream &in) {
            return in.good() && in.rdbuf()->sgetc() != std::ifstream::traits_type::eof();
        }
    } // namespace

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
        bool darkTheme = darkSettingsMode();
        bool lightTimeline = timelineLightMode();
        IOUtilities::readAndDecode(in, &darkTheme);
        IOUtilities::readAndDecode(in, &lightTimeline);
        if (in.fail()) {
            return;
        }
        darkSettingsModeFlag() = darkTheme;
        timelineLightModeFlag().store(lightTimeline, std::memory_order_relaxed);
        uint8_t storedLanguageId = 0;
        if (hasMoreBytes(in)) {
            IOUtilities::readAndDecode(in, &storedLanguageId);
        }
        const bool useJapanese = !in.fail() && storedLanguageId == static_cast<uint8_t>(Language::Japanese);
        UiLanguage::load(useJapanese ? Language::Japanese : Language::English);

        if (hasMoreBytes(in)) {
            bool useLegacySettingsUI = legacySettingsUI();
            IOUtilities::readAndDecode(in, &useLegacySettingsUI);
            if (!in.fail()) {
                legacySettingsUI() = useLegacySettingsUI;
            }
        }

        showSettingDescriptions() = true;
        if (hasMoreBytes(in)) {
            bool showDescriptions = true;
            IOUtilities::readAndDecode(in, &showDescriptions);
            if (!in.fail()) {
                showSettingDescriptions() = showDescriptions;
            }
        }
    }

    void PreferencesIO::save() {
        const std::filesystem::path path = preferencesFile();
        std::error_code directoryError;
        std::filesystem::create_directories(path.parent_path(), directoryError);
        std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            return;
        }
        IOUtilities::encodeAndWrite(out, MAGIC);
        IOUtilities::encodeAndWrite(out, VERSION);
        IOUtilities::encodeAndWrite(out, darkSettingsMode());
        IOUtilities::encodeAndWrite(out, timelineLightMode());
        IOUtilities::encodeAndWrite(out, static_cast<uint8_t>(UiLanguage::preferred()));
        IOUtilities::encodeAndWrite(out, legacySettingsUI());
        IOUtilities::encodeAndWrite(out, showSettingDescriptions());
    }
} // namespace merutilm::rff2
