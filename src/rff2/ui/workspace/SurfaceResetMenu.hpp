//
// Modified by GPT-6 on 2026-09-14, 2026-09-22
//

#pragma once
#include "SurfaceInspectorContent.hpp"
#include "SurfaceEditHistory.hpp"
#include "AttributeFormModel.hpp"
#include <windows.h>

namespace merutilm::rff2::workspace {
    struct SurfaceResetMenu {
        enum Command { VALUE = 1, EFFECT = 2, ADDED = 3, APPEARANCE = 4 };
        struct Item {
            int command;
            std::wstring label;
            bool enabled;
        };
        static std::vector<Item> items(const ShdSlopeAttribute &value, Category category,
                                       const SurfaceInspectorContent::Row *row, bool all) {
            const auto defaults = surfaceDefaults();
            bool valueChanged = false;
            std::wstring singleResetLabel = L"Reset value (select a control)";
            if (row && row->parameter) {
                const auto &parameter = *row->parameter;
                valueChanged = parameter.value(value) != parameter.value(defaults);
                singleResetLabel = L"Reset " + std::wstring(displayLabel(parameter)) + L" to " +
                                   AttributeFormModel::number(parameter.value(defaults));
            }
            if (row && row->color) {
                const auto &color = *row->color;
                valueChanged = value.*color.member != defaults.*color.member;
                singleResetLabel = L"Reset " + std::wstring(color.label) + L" to default color";
            }
            return {
                {VALUE, singleResetLabel, valueChanged},
                {EFFECT, L"Reset effect: " + categoryLabel(category),
                 SurfaceEditHistory::canResetGroup(value, category)},
                {ADDED, L"Clear added effects (keep base style)", SurfaceEditHistory::hasAddedEffects(value)},
                {APPEARANCE, L"Reset all appearance (includes palette)", all}};
        }
        static int show(HWND owner, POINT point, const std::vector<Item> &items, UINT dpi);
        static bool measure(HWND owner, MEASUREITEMSTRUCT *item);
        static bool draw(const DRAWITEMSTRUCT *item);
    };
} // namespace merutilm::rff2::workspace
