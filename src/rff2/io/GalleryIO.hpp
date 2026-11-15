//
// Created by Opus 5.5 on 2026-11-07
// Modified by Opus 5.5 on 2026-11-09, 2026-11-10, 2026-11-13, 2026-11-15
//

#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <windows.h>

namespace merutilm::rff2 {
    // What the Gallery keeps between runs - the folder it showed, where its window was, and the files
    // used most recently - in config/gallery.json, and what it can tell about one saved file.
    struct GalleryIO {
        GalleryIO() = delete;

        // The kinds of file the Gallery lists. Settings and Location both reopen a view.
        enum class Kind : uint8_t { SETTINGS, LOCATION, MAP, IMAGE, APPEARANCE, COUNT };

        static constexpr size_t MAX_RECENT = 40;

        struct State {
            std::filesystem::path folder;
            RECT window{};
            bool hasWindow = false;
            bool maximized = false;
            uint8_t filter = 0;
            uint8_t sort = 0;
            // The share of the Gallery's left side given to the folders beside the one shown, above Recent Folders.
            float folderSplit = 0.6f;
            // Gallery (Open) closes once a file has opened from it.
            bool closeAfterOpen = true;
            // Cards and the details panel name the files by their extensions, such as .rfc and .rfth, instead of by kind.
            bool showExtensions = false;
            // Save Work in Gallery (Save) writes the compressed map beside the settings and the image.
            bool saveMapWithWork = false;
            std::vector<std::filesystem::path> recent;
        };

        // The state read from disk on first use and kept live after that.
        static State &state();

        static void save();

        // Puts file at the top of the recent list, without a second entry for it, and saves the list.
        static void addRecent(const std::filesystem::path &file);

        // Which kind of file path is, by its extension.
        [[nodiscard]] static std::optional<Kind> kindOf(const std::filesystem::path &path);

        // Label and value pairs describing the file, for the Gallery's details panel. English; translated where shown.
        [[nodiscard]] static std::vector<std::pair<std::wstring, std::wstring>> describe(const std::filesystem::path &path);

        // A magnification written for reading, such as 1.23e45, from the base-10 logarithm the files store.
        [[nodiscard]] static std::wstring zoomText(double logZoom);
    };
}
