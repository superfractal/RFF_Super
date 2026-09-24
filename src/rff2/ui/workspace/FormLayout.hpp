//
// Modified by GPT-6 on 2026-09-14, 2026-09-19, 2026-09-21, 2026-09-23
//

#pragma once
#include <algorithm>

namespace merutilm::rff2::workspace {
    struct FormLayout {
        static constexpr int inset = 20;
        static constexpr int firstRow = 64;
        static constexpr int rowPitch = 96;
        static constexpr int inputOffset = 24;
        static constexpr int inputHeight = 28;
        static constexpr int actionPitch = 40;
        static constexpr int footerHeight = 104;
        static constexpr int headerHeight = 52;
        static constexpr int pickerWidth = 80;
        static constexpr int pickerGap = 8;

        struct Row {
            int labelHeight;
            int inputOffset;
            int hintOffset;
            int height;
        };

        static Row measuredRow(int labelPixels, int hintPixels, float scale) {
            const auto px = [scale](int value) { return int(value * scale + 0.5f); };
            const int labelHeight = std::max(px(20), labelPixels);
            const int inputTop = labelHeight + px(4);
            const int hintTop = inputTop + px(inputHeight) + px(4);
            const int fieldBottom = hintPixels ? hintTop + hintPixels : inputTop + px(inputHeight);
            const int minimumHeight = hintPixels ? px(rowPitch) : 0;
            return {labelHeight, inputTop, hintTop, std::max(minimumHeight, fieldBottom + px(12))};
        }

        static int rowTop(int index) {
            return firstRow + index * rowPitch;
        }

        static int actionsTop(int rows) {
            return firstRow + rows * rowPitch + 12;
        }
    };
}
