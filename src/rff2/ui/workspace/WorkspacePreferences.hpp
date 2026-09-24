//
// Modified by GPT-6 on 2026-09-13, 2026-09-14, 2026-09-17, 2026-09-22
//

#pragma once
#include "EffectNavigation.hpp"
#include "WorkspaceGeometry.hpp"
#include "AppearancePanelLayout.hpp"
#include <array>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <windows.h>

namespace merutilm::rff2::workspace {
    class WorkspacePreferences {
        static constexpr std::array<std::string_view, 10> keys = {
            "color", "reflection", "film", "contour", "emission", "flame", "print", "noise", "relief", "mix"};

      public:
        static bool load(const std::filesystem::path &path, EffectNavigation &navigation,
                         PaneWidths *widths = nullptr, AppearancePanelLayout *panels = nullptr) {
            std::ifstream input(path);
            std::string line;
            if (!std::getline(input, line) || line != "RFF_WORKSPACE_FAVORITES_1") {
                return false;
            }
            EffectNavigation loadedNavigation;
            PaneWidths loadedWidths = widths ? *widths : PaneWidths{};
            AppearancePanelLayout loadedPanels;
            while (std::getline(input, line)) {
                loadedPanels.read(line);
                const auto readWidth = [&](std::string_view prefix, int &target, int minimum, int maximum) {
                    if (!line.starts_with(prefix)) {
                        return;
                    }
                    int value = 0;
                    const auto lineEnd = line.data() + line.size();
                    const auto parsed = std::from_chars(line.data() + prefix.size(), lineEnd, value);
                    if (parsed.ec == std::errc{} && parsed.ptr == lineEnd && value >= minimum &&
                        value <= maximum) {
                        target = value;
                    }
                };
                readWidth("navigation-width=", loadedWidths.navigation, PaneWidths::minimumNavigation,
                          PaneWidths::maximumNavigation);
                readWidth("inspector-width=", loadedWidths.inspector, PaneWidths::minimumInspector,
                          PaneWidths::maximumInspector);
                if (line.starts_with("recent=")) {
                    const auto key = std::string_view(line).substr(7);
                    const auto entry = std::find(keys.begin(), keys.end(), key);
                    if (entry != keys.end()) {
                        loadedNavigation.remember(static_cast<Category>(entry - keys.begin()));
                    }
                    continue;
                }
                const auto found = std::find(keys.begin(), keys.end(), line);
                if (found == keys.end()) {
                    continue;
                }
                const auto category = static_cast<Category>(found - keys.begin());
                if (!loadedNavigation.favorite(category)) {
                    loadedNavigation.toggleFavorite(category);
                }
            }
            if (input.bad()) {
                return false;
            }
            loadedNavigation.filter = navigation.filter;
            navigation = loadedNavigation;
            if (widths) {
                *widths = loadedWidths;
            }
            if (panels) {
                *panels = loadedPanels;
            }
            return true;
        }

        static bool save(const std::filesystem::path &path, const EffectNavigation &navigation,
                         const PaneWidths *widths = nullptr, const AppearancePanelLayout *panels = nullptr) {
            if (path.empty()) {
                return true;
            }
            auto temporary = path;
            temporary += L"." + std::to_wstring(GetCurrentProcessId()) + L".tmp";
            {
                std::ofstream output(temporary, std::ios::trunc);
                if (!output) {
                    return false;
                }
                output << "RFF_WORKSPACE_FAVORITES_1\n";
                for (size_t i = 0; i < keys.size(); ++i) {
                    if (navigation.favorite(static_cast<Category>(i))) {
                        output << keys[i] << '\n';
                    }
                }
                for (auto recentCategory = navigation.recentCategories().rbegin();
                     recentCategory != navigation.recentCategories().rend(); ++recentCategory) {
                    output << "recent=" << keys[size_t(*recentCategory)] << '\n';
                }
                if (widths) {
                    auto normalizedWidths = *widths;
                    normalizedWidths.normalize();
                    output << "navigation-width=" << normalizedWidths.navigation << '\n'
                           << "inspector-width=" << normalizedWidths.inspector << '\n';
                }
                if (panels) {
                    panels->write(output);
                }
                output.close();
                if (!output) {
                    DeleteFileW(temporary.c_str());
                    return false;
                }
            }
            if (MoveFileExW(temporary.c_str(), path.c_str(),
                            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
                return true;
            }
            DeleteFileW(temporary.c_str());
            return false;
        }
    };
}
