//
// Modified by GPT-6 on 2026-09-14, 2026-09-15, 2026-09-18, 2026-09-22
//

#pragma once
#include <algorithm>
#include <array>
#include <windows.h>

namespace merutilm::rff2::workspace {
    struct HeaderLayout {
        int height = 64;
        bool stacked = false, chooser = false;
        RECT search{}, module{}, navigation{}, workspace{}, inspector{};
        std::array<RECT, 4> tabs{};

        static HeaderLayout arrange(int width, float scale) {
            HeaderLayout result;
            const auto pixels = [scale](int value) { return int(value * scale + .5f); };
            const auto box = [&](int x, int y, int boxWidth, int boxHeight) {
                return RECT{pixels(x), pixels(y), pixels(x + boxWidth), pixels(y + boxHeight)};
            };
            result.stacked = width < pixels(1180);
            result.chooser = width < pixels(500);
            int headerHeight = 64;
            if (result.chooser) {
                headerHeight = 204;
            } else if (width < pixels(760)) {
                headerHeight = 156;
            } else if (result.stacked) {
                headerHeight = 112;
            }
            result.height = pixels(headerHeight);
            for (int tabIndex = 0; tabIndex < 4; ++tabIndex) {
                result.tabs[tabIndex] = box(20 + tabIndex * 115, 10, 110, 40);
            }
            if (!result.stacked) {
                result.module = box(500, 16, 200, 28);
                result.search = {std::max(pixels(720), width - pixels(588)), pixels(12), width - pixels(288),
                                 pixels(48)};
            } else if (width >= pixels(760)) {
                result.module = box(20, 64, 200, 28);
                result.search = {std::max(pixels(240), width - pixels(588)), pixels(60), width - pixels(288),
                                 pixels(96)};
            } else {
                if (result.chooser) {
                    result.module = {pixels(20), pixels(64), width - pixels(20), pixels(92)};
                    result.search = {pixels(20), pixels(108), width - pixels(20), pixels(144)};
                } else {
                    result.module = box(20, 64, 200, 28);
                    result.search = {std::max(pixels(240), width - pixels(320)), pixels(60),
                                     width - pixels(20), pixels(96)};
                }
            }
            int toggleY = 12;
            if (result.chooser) {
                toggleY = 156;
            } else if (width < pixels(760)) {
                toggleY = 108;
            } else if (result.stacked) {
                toggleY = 60;
            }
            result.navigation = {width - pixels(268), pixels(toggleY), width - pixels(144),
                                 pixels(toggleY + 36)};
            result.inspector = {width - pixels(132), pixels(toggleY), width - pixels(20),
                                pixels(toggleY + 36)};
            if (result.chooser) {
                result.workspace = {pixels(20), pixels(16), width - pixels(20), pixels(44)};
            }
            return result;
        }
        static bool contains(const RECT &rect, int x, int y) {
            return x >= rect.left && x < rect.right && y >= rect.top && y < rect.bottom;
        }
    };
}
