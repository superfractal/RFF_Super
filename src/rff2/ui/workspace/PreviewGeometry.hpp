//
// Modified by GPT-6 on 2026-09-14, 2026-09-22
//

#pragma once
#include <algorithm>
#include <cstdint>
#include <windows.h>

namespace merutilm::rff2::workspace {
    struct PreviewGeometry {
        static RECT fit(RECT available, SIZE image) {
            const int availableWidth = std::max(1L, available.right - available.left),
                      availableHeight = std::max(1L, available.bottom - available.top);
            const int imageWidth = std::max(1L, image.cx), imageHeight = std::max(1L, image.cy);
            int displayWidth = availableWidth, displayHeight = availableHeight;
            if (int64_t(availableWidth) * imageHeight > int64_t(availableHeight) * imageWidth) {
                displayWidth = std::max(1, int(int64_t(availableHeight) * imageWidth / imageHeight));
            } else {
                displayHeight = std::max(1, int(int64_t(availableWidth) * imageHeight / imageWidth));
            }
            const int x = available.left + (availableWidth - displayWidth) / 2,
                      y = available.top + (availableHeight - displayHeight) / 2;
            return {x, y, x + displayWidth, y + displayHeight};
        }
        static uint16_t sample(int position, int displayed, int samples, bool flipped = false) {
            if (displayed <= 0 || samples <= 0) {
                return 0;
            }
            const int pixel = int(int64_t(std::clamp(position, 0, displayed - 1)) * samples / displayed);
            return uint16_t(flipped ? samples - 1 - pixel : pixel);
        }
    };
} // namespace merutilm::rff2::workspace
