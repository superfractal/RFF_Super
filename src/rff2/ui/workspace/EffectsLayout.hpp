//
// Modified by GPT-6 on 2026-09-13, 2026-09-14, 2026-09-15, 2026-09-18, 2026-09-22
//

#pragma once
#include "EffectNavigation.hpp"
#include <algorithm>
#include <windows.h>

namespace merutilm::rff2::workspace {
    struct EffectsLayout {
        struct Box {
            int x, y, width, height;
            RECT pixels(float scale) const {
                return {int(x * scale + .5f), int(y * scale + .5f), int((x + width) * scale + .5f),
                        int((y + height) * scale + .5f)};
            }
            bool contains(int pointX, int pointY, float scale) const {
                const RECT pixelBounds = pixels(scale);
                return pointX >= pixelBounds.left && pointX < pixelBounds.right &&
                       pointY >= pixelBounds.top && pointY < pixelBounds.bottom;
            }
        };
        enum class Kind { NONE, STUDIO, FILTER, CATEGORY, FAVORITE, PALETTE };
        struct Target {
            Kind kind = Kind::NONE;
            int index = 0;
            bool operator==(const Target &) const = default;
        };
        static constexpr int categoryTop = 216;
        static constexpr int categoryStep = 56;
        static constexpr Box studio(int width = 220) {
            return {width - 92, 64, 84, 28};
        }
        static constexpr Box filter(int filterIndex, int width = 220) {
            const int filterStep = std::min((width - 16) / 3, (width - 88) / 2),
                      filterLeft = 8 + filterIndex * filterStep;
            return {filterLeft, 156,
                    (filterIndex == 2 ? width - 8 : 8 + (filterIndex + 1) * filterStep) - 2 - filterLeft, 36};
        }
        static constexpr Box row(int rowIndex, int width = 220) {
            return {8, categoryTop + rowIndex * categoryStep, width - 16, 50};
        }
        static constexpr Box category(int rowIndex, int width = 220) {
            auto rowBox = row(rowIndex, width);
            rowBox.width -= 26;
            return rowBox;
        }
        static constexpr Box favorite(int rowIndex, int width = 220) {
            const auto rowBox = row(rowIndex, width);
            return {rowBox.x + rowBox.width - 26, rowBox.y, 26, rowBox.height};
        }
        static constexpr Box label(int rowIndex, int width = 220) {
            const auto rowBox = category(rowIndex, width);
            return {rowBox.x + 12, rowBox.y + 4, rowBox.width - 20, 22};
        }
        static constexpr Box status(int rowIndex, int width = 220) {
            auto rowBox = label(rowIndex, width);
            rowBox.y += 24;
            rowBox.height = 18;
            return rowBox;
        }
        static Box palette(int categoryCount, int width = 220) {
            return {20, categoryTop + std::max(1, categoryCount) * categoryStep + 8, width - 34, 36};
        }
        static Target hit(int pointX, int pointY, int categoryCount, float scale, int width = 220) {
            if (studio(width).contains(pointX, pointY, scale)) {
                return {Kind::STUDIO};
            }
            for (int filterIndex = 0; filterIndex < EffectNavigation::filterCount; ++filterIndex) {
                if (filter(filterIndex, width).contains(pointX, pointY, scale)) {
                    return {Kind::FILTER, filterIndex};
                }
            }
            for (int categoryIndex = 0; categoryIndex < categoryCount; ++categoryIndex) {
                if (favorite(categoryIndex, width).contains(pointX, pointY, scale)) {
                    return {Kind::FAVORITE, categoryIndex};
                }
                if (category(categoryIndex, width).contains(pointX, pointY, scale)) {
                    return {Kind::CATEGORY, categoryIndex};
                }
            }
            if (palette(categoryCount, width).contains(pointX, pointY, scale)) {
                return {Kind::PALETTE};
            }
            return {};
        }
    };
} // namespace merutilm::rff2::workspace
