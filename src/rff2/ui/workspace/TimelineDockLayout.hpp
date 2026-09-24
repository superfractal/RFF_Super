//
// Modified by GPT-6 on 2026-09-14, 2026-09-18, 2026-09-19, 2026-09-22, 2026-09-23
//

#pragma once
#include "../UiDpi.hpp"
#include <algorithm>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace merutilm::rff2::workspace {
    struct TimelineDockState {
        int height = 200;
        bool collapsed = false;
        bool previewHidden = false;
        int previewPercent = 50;
        int inspectorWidth = 340;
        bool inspectorOpen = true;
        bool inspectorLeft = false;
        static constexpr int minimumHeight = 160, maximumHeight = 4096;
        bool operator==(const TimelineDockState&) const = default;

        void load(const std::filesystem::path& path) {
            std::ifstream file(path);
            std::string line;
            if (!std::getline(file, line) || line != "RFF_TIMELINE_LAYOUT_1") {
                return;
            }
            auto loaded = *this;
            loaded.previewPercent = 0;
            loaded.previewHidden = false;
            while (std::getline(file, line)) {
                if (line == "collapsed=0") {
                    loaded.collapsed = false;
                } else if (line == "collapsed=1") {
                    loaded.collapsed = true;
                } else if (line == "preview-hidden=0") {
                    loaded.previewHidden = false;
                } else if (line == "preview-hidden=1") {
                    loaded.previewHidden = true;
                } else if (line == "inspector-open=0") {
                    loaded.inspectorOpen = false;
                } else if (line == "inspector-open=1") {
                    loaded.inspectorOpen = true;
                } else if (line == "inspector-left=0") {
                    loaded.inspectorLeft = false;
                } else if (line == "inspector-left=1") {
                    loaded.inspectorLeft = true;
                }

                std::string_view prefix;
                if (line.starts_with("inspector-width=")) {
                    prefix = "inspector-width=";
                } else if (line.starts_with("preview-percent=")) {
                    prefix = "preview-percent=";
                } else if (line.starts_with("height=")) {
                    prefix = "height=";
                } else {
                    continue;
                }

                int value = 0;
                const auto end = line.data() + line.size();
                const auto parsed = std::from_chars(line.data() + prefix.size(), end, value);
                if (parsed.ec != std::errc{} || parsed.ptr != end) {
                    continue;
                }
                if (prefix == "height=" && value >= minimumHeight && value <= maximumHeight) {
                    loaded.height = value;
                } else if (prefix == "inspector-width=" && value >= 300 && value <= 640) {
                    loaded.inspectorWidth = value;
                } else if (prefix == "preview-percent=" && (value == 0 || (value >= 10 && value <= 90))) {
                    loaded.previewPercent = value;
                }
            }
            if (loaded.collapsed) {
                loaded.previewHidden = false;
            }
            if (!file.bad()) {
                *this = loaded;
            }
        }
        bool save(const std::filesystem::path& path) const {
            if (path.empty()) {
                return true;
            }
            auto temporary = path;
            temporary += L"." + std::to_wstring(GetCurrentProcessId()) + L".tmp";
            {
                std::ofstream file(temporary, std::ios::trunc);
                if (!file) {
                    return false;
                }
                file << "RFF_TIMELINE_LAYOUT_1\nheight=" << std::clamp(height, minimumHeight, maximumHeight)
                     << "\ncollapsed=" << (collapsed ? 1 : 0) << '\n';
                file << "preview-hidden=" << (!collapsed && previewHidden ? 1 : 0)
                     << "\npreview-percent=" << previewPercent << '\n';
                file << "inspector-width=" << std::clamp(inspectorWidth, 300, 640)
                     << "\ninspector-open=" << (inspectorOpen ? 1 : 0) << '\n';
                file << "inspector-left=" << (inspectorLeft ? 1 : 0) << '\n';
                file.close();
                if (!file) {
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

    struct TimelineDockLayout {
        RECT preview{}, transport{}, tracks{}, divider{}, toggle{};
        bool tracksVisible = false, tracksAvailable = false;
        int maximumTracksHeight = 0, paneSpace = 0;
        static TimelineDockLayout arrange(int width, int height, UINT dpi, const TimelineDockState& state,
                                          int header = 0, int minimumTracks = TimelineDockState::minimumHeight) {
            const auto px = [dpi](int value) { return UiDpi::pixels(value, dpi); };
            const bool narrow = width < px(700);
            width = std::max(0, width);
            height = std::max(0, height);
            const int margin = std::min({px(12), width / 2, height / 2});
            int headerHeight = header;
            if (headerHeight == 0) {
                headerHeight = width < px(560) ? 108 : 60;
            }
            const int top = std::min(std::max(0, height - margin), px(headerHeight));
            const int bottom = std::max(top, height - margin);
            const int transportHeight = px(narrow ? 100 : 56);
            const int dividerHeight = px(8);
            const int availablePaneHeight =
                std::max(0, bottom - top - transportHeight - margin - dividerHeight);
            TimelineDockLayout result;
            // A short editor gives the preview space to tracks until Hide Tracks restores it.
            const bool compactTracks = availablePaneHeight >= px(minimumTracks) &&
                                       availablePaneHeight < px(minimumTracks + 96);
            result.paneSpace = availablePaneHeight;
            result.maximumTracksHeight =
                std::max(0, availablePaneHeight - (compactTracks || state.previewHidden ? 0 : px(96)));
            result.tracksAvailable = result.maximumTracksHeight >= px(minimumTracks);
            const int preferredTracksHeight = px(std::clamp(state.height, TimelineDockState::minimumHeight,
                                                             TimelineDockState::maximumHeight));
            int desiredTracksHeight = preferredTracksHeight;
            if (state.previewPercent != 0) {
                desiredTracksHeight = availablePaneHeight *
                                      (100 - std::clamp(state.previewPercent, 10, 90)) / 100;
            }
            int tracksHeight = 0;
            if (!state.collapsed && result.tracksAvailable) {
                if (compactTracks || state.previewHidden) {
                    tracksHeight = result.maximumTracksHeight;
                } else {
                    tracksHeight = std::clamp(desiredTracksHeight, px(minimumTracks), result.maximumTracksHeight);
                }
            }
            result.tracksVisible = tracksHeight >= px(minimumTracks);
            const int visibleTracksHeight = result.tracksVisible ? tracksHeight : 0;
            result.tracks = {margin, bottom - visibleTracksHeight, std::max(margin, width - margin), bottom};
            const int transportBottom = bottom - (result.tracksVisible ? visibleTracksHeight + margin : 0);
            result.transport = {margin, std::max(top, transportBottom - transportHeight),
                                std::max(margin, width - margin), transportBottom};
            result.preview = {margin, top, std::max(margin, width - margin),
                              std::max<LONG>(top, result.transport.top - dividerHeight - margin)};
            if (state.previewHidden && result.tracksVisible) {
                result.preview.bottom = result.preview.top;
            }
            if (result.tracksVisible && !compactTracks && !state.previewHidden) {
                result.divider = {margin, result.transport.top - dividerHeight,
                                  std::max(margin, width - margin), result.transport.top};
            }
            result.toggle = {std::max<LONG>(margin, result.transport.right - px(116)),
                             result.transport.top + px(14), result.transport.right - px(8),
                             result.transport.top + px(42)};
            return result;
        }
    };
}
