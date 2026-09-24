//
// Modified by GPT-6 on 2026-09-14, 2026-09-22, 2026-09-23
//

#pragma once
#include "../UiDpi.hpp"
#include <algorithm>
#include <array>
#include <cstddef>

namespace merutilm::rff2::workspace {
    struct TimelineTransportLayout {
        std::array<RECT, 4> buttons{};
        std::array<RECT, 4> fields{};
        RECT separator{};
        RECT status{};

        static TimelineTransportLayout arrange(RECT area, int right, UINT dpi, bool narrow, int timeTextWidth,
                                               std::array<int, 4> order, int minimumReadoutHeight = 0) {
            const auto px = [dpi](int value) { return UiDpi::pixels(value, dpi); };
            TimelineTransportLayout result;
            const int padding = px(12);
            const int buttonSize = px(30);
            const int gap = px(6);
            const int buttonMiddle = narrow ? area.top + px(28) : (area.top + area.bottom) / 2;
            const int buttonTop = buttonMiddle - buttonSize / 2;
            int nextX = area.left + padding;
            for (size_t buttonIndex = 0; buttonIndex < result.buttons.size(); ++buttonIndex) {
                result.buttons[buttonIndex] = {nextX, buttonTop, nextX + buttonSize,
                                               buttonTop + buttonSize};
                nextX += buttonSize + (buttonIndex == 2 ? padding : gap);
            }
            const int separatorX = result.buttons.back().right + padding;
            result.separator = {separatorX, area.top + px(10), separatorX + 1,
                                narrow ? area.top + px(46) : area.bottom - px(10)};
            nextX = narrow ? area.left + padding : separatorX + padding;
            const int availableWidth = std::max(0, right - nextX);
            const int fieldGap = px(8);
            const int statusWidth = !narrow && availableWidth >= px(600) ? px(108) : 0;
            const int timeWidth = std::max(px(120), timeTextWidth + px(16));
            const int fieldWidth =
                std::clamp((availableWidth - statusWidth - timeWidth - fieldGap * 3) / 3, px(84), px(154));
            const int fieldMiddle = narrow ? area.top + px(74) : buttonMiddle;
            const int measuredHeight = std::max(px(40), minimumReadoutHeight);
            int fieldHeight = measuredHeight;
            if (fieldHeight % 2 != buttonSize % 2) {
                ++fieldHeight;
            }
            const int fieldTop = fieldMiddle - fieldHeight / 2;
            if (!narrow && availableWidth < px(84) * 3 + timeWidth + fieldGap * 3) {
                order = {0, 3, 1, 2};
            }
            for (int fieldIndex : order) {
                const int currentFieldWidth = fieldIndex == 3 ? std::max(fieldWidth, timeWidth) : fieldWidth;
                if (nextX + currentFieldWidth > right) {
                    break;
                }
                result.fields[fieldIndex] = {nextX, fieldTop, nextX + currentFieldWidth,
                                             fieldTop + fieldHeight};
                nextX += currentFieldWidth + fieldGap;
            }
            result.status = {std::min(nextX, right), area.top, right, area.bottom};
            return result;
        }
    };
} // namespace merutilm::rff2::workspace
