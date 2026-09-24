//
// Modified by GPT-6 on 2026-09-14, 2026-09-18, 2026-09-22
//

#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <windows.h>

namespace merutilm::rff2::workspace {
    struct InspectorScroll {
        RECT track{}, thumb{};
        int maximum = 0;
        bool visible() const {
            return maximum > 0 && track.bottom > track.top;
        }
        static InspectorScroll content(int width, int height, float scale, int contentHeight, int position,
                                       int rightInset = 8) {
            const auto pixels = [scale](int value) { return int(value * scale + .5f); };
            InspectorScroll scrollbar;
            scrollbar.maximum = std::max(0, contentHeight - height);
            scrollbar.track = {width - pixels(rightInset + 4), pixels(8), width - pixels(rightInset),
                               std::max(pixels(8), height - pixels(8))};
            const int trackHeight = scrollbar.track.bottom - scrollbar.track.top;
            const int thumbHeight = std::min(
                trackHeight,
                std::max(pixels(20), int(int64_t(trackHeight) * height / std::max(1, contentHeight))));
            int thumbOffset = 0;
            if (scrollbar.maximum) {
                thumbOffset =
                    int(std::lround((trackHeight - thumbHeight) *
                                    double(std::clamp(position, 0, scrollbar.maximum)) / scrollbar.maximum));
            }
            scrollbar.thumb = {scrollbar.track.left, scrollbar.track.top + thumbOffset, scrollbar.track.right,
                               scrollbar.track.top + thumbOffset + thumbHeight};
            return scrollbar;
        }
        static InspectorScroll layout(int width, int height, float scale, int footer, int page, int count,
                                      int position) {
            auto pixels = [scale](int value) { return int(value * scale + .5f); };
            InspectorScroll scrollbar;
            scrollbar.maximum = std::max(0, count - std::max(1, page));
            scrollbar.track = {width - pixels(12), pixels(152), width - pixels(8),
                               std::max(pixels(152), height - pixels(footer))};
            const int trackHeight = scrollbar.track.bottom - scrollbar.track.top;
            const int thumbHeight = std::min(
                trackHeight, std::max(pixels(20), trackHeight * std::max(1, page) / std::max(1, count)));
            int thumbOffset = 0;
            if (scrollbar.maximum) {
                thumbOffset =
                    int(std::lround((trackHeight - thumbHeight) *
                                    double(std::clamp(position, 0, scrollbar.maximum)) / scrollbar.maximum));
            }
            scrollbar.thumb = {scrollbar.track.left, scrollbar.track.top + thumbOffset, scrollbar.track.right,
                               scrollbar.track.top + thumbOffset + thumbHeight};
            return scrollbar;
        }
        int positionAt(int pointerY, int grabOffset) const {
            const int thumbTravel = track.bottom - track.top - (thumb.bottom - thumb.top);
            if (thumbTravel <= 0) {
                return 0;
            }
            return std::clamp(
                int(std::lround(double(pointerY - grabOffset - track.top) * maximum / thumbTravel)), 0,
                maximum);
        }
    };
}
