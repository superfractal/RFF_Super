//
// Modified by GPT-6 on 2026-09-13, 2026-09-14, 2026-09-15, 2026-09-18, 2026-09-22
// Modified by Opus 5.5 on 2026-11-01
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
        static constexpr int filterTop = 156;
        static constexpr int categoryTop = 216;
        static constexpr int categoryStep = 56;
        static constexpr Box studio(int width = 220) {
            return {width - 92, 64, 84, 28};
        }
        static constexpr Box filter(int filterIndex, int width = 220) {
            const int filterStep = std::min((width - 16) / 3, (width - 88) / 2),
                      filterLeft = 8 + filterIndex * filterStep;
            return {filterLeft, filterTop,
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
        // A settings page's section list is the Surface Effects list moved up to where its Base Style row starts, so tabs, rows and stars share its sizes and gaps.
        static constexpr int sectionShift = 64 - filterTop, sectionTop = categoryTop + sectionShift;
        static constexpr Box shifted(Box box) {
            box.y += sectionShift;
            return box;
        }
        // A page with fewer sections than this keeps its plain list: rows 36 high and 40 apart under the title, no tabs or stars.
        static constexpr int minimumTabbedSections = 4, plainTop = 64, plainStep = 40;
        static constexpr Box sectionFilter(int slot, int width = 220) {
            return shifted(filter(slot, width));
        }
        static constexpr Box sectionRow(int rowIndex, int width, bool tabbed) {
            return tabbed ? shifted(row(rowIndex, width)) : Box{10, plainTop + rowIndex * plainStep, width - 20, 36};
        }
        static constexpr Box section(int rowIndex, int width, bool tabbed) {
            return tabbed ? shifted(category(rowIndex, width)) : sectionRow(rowIndex, width, false);
        }
        static constexpr Box sectionFavorite(int rowIndex, int width = 220) {
            return shifted(favorite(rowIndex, width));
        }
        static constexpr int sectionListBottom(int rowCount, bool tabbed) {
            return tabbed ? sectionTop + std::max(1, rowCount) * categoryStep : plainTop + rowCount * plainStep;
        }
        // tabCount 0 is the plain list, which has no hover targets.
        static Target sectionHit(int pointX, int pointY, int rowCount, int tabCount, float scale, int width = 220) {
            if (tabCount == 0) {
                return {};
            }
            for (int slot = 0; slot < tabCount; ++slot) {
                if (sectionFilter(slot, width).contains(pointX, pointY, scale)) {
                    return {Kind::FILTER, slot};
                }
            }
            for (int rowIndex = 0; rowIndex < rowCount; ++rowIndex) {
                if (sectionFavorite(rowIndex, width).contains(pointX, pointY, scale)) {
                    return {Kind::FAVORITE, rowIndex};
                }
                if (section(rowIndex, width, true).contains(pointX, pointY, scale)) {
                    return {Kind::CATEGORY, rowIndex};
                }
            }
            return {};
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
