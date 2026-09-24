//
// Modified by GPT-6 on 2026-09-14, 2026-09-15, 2026-09-18, 2026-09-22
//

#pragma once
#include "AccessibleItems.hpp"
#include "EffectsLayout.hpp"
#include "EffectNavigation.hpp"

namespace merutilm::rff2::workspace {
    struct NavigationItems {
        static constexpr long studio = 1, style = 2, filter = 10, category = 100, favorite = 101,
                              palette = 200, section = 1000;
        static long categoryId(Category value) {
            return category + 2 * long(value);
        }
        static long favoriteId(Category value) {
            return favorite + 2 * long(value);
        }
        static std::vector<AccessibleItem> effects(const ShdSlopeAttribute &value,
                                                   const EffectNavigation &navigation, Category selected,
                                                   int width, float scale) {
            std::vector<AccessibleItem> result;
            const auto add = [&](long id, std::wstring name, std::wstring help, std::wstring action,
                                 EffectsLayout::Box rect, long role, long flags = 0) {
                result.push_back({id, std::move(name), std::move(help), std::move(action), rect.pixels(scale),
                                  role, STATE_SYSTEM_FOCUSABLE | flags});
            };
            add(studio, L"Surface Studio", L"Enable or bypass the surface effects.",
                value.studio.use ? L"Turn off" : L"Turn on", EffectsLayout::studio(width),
                ROLE_SYSTEM_CHECKBUTTON, value.studio.use ? STATE_SYSTEM_CHECKED : 0);
            add(style, L"Base Style", L"Choose the starting surface style.", L"Open",
                {18, 104, width - 34, 28}, ROLE_SYSTEM_COMBOBOX);
            for (int filterIndex = 0; filterIndex < EffectNavigation::filterCount; ++filterIndex) {
                add(filter + filterIndex, std::wstring(EffectNavigation::filterLabels[filterIndex]),
                    std::wstring(EffectNavigation::filterHelp[filterIndex]), L"Select",
                    EffectsLayout::filter(filterIndex, width), ROLE_SYSTEM_RADIOBUTTON,
                    int(navigation.filter) == filterIndex ? STATE_SYSTEM_CHECKED : 0);
            }
            const auto categories = navigation.rows(value);
            for (int categoryIndex = 0; categoryIndex < int(categories.size()); ++categoryIndex) {
                const auto current = categories[categoryIndex];
                const auto row = EffectsLayout::category(categoryIndex, width);
                auto summary = navigation.stateLabel(value, current);
                if (summary.empty()) {
                    summary = effectState(value, current).label();
                }
                add(categoryId(current), categoryLabel(current), summary + L". Open this effect's settings.",
                    L"Open", row, ROLE_SYSTEM_LISTITEM,
                    STATE_SYSTEM_SELECTABLE | (current == selected ? STATE_SYSTEM_SELECTED : 0));
                add(favoriteId(current), L"Favorite " + categoryLabel(current),
                    L"Include this effect in Favorites. This does not change the artwork.",
                    navigation.favorite(current) ? L"Remove favorite" : L"Add favorite",
                    EffectsLayout::favorite(categoryIndex, width), ROLE_SYSTEM_CHECKBUTTON,
                    navigation.favorite(current) ? STATE_SYSTEM_CHECKED : 0);
            }
            if (categories.empty()) {
                const auto text = navigation.emptyText();
                result.push_back(
                    {150, std::wstring(text[0]) + L". " + std::wstring(text[1]), L"", L"",
                     EffectsLayout::Box{20, EffectsLayout::categoryTop, width - 34, 40}.pixels(scale),
                     ROLE_SYSTEM_STATICTEXT, 0});
            }
            add(palette, L"Edit Palette", L"Open the palette editor.", L"Open",
                EffectsLayout::palette(int(categories.size()), width), ROLE_SYSTEM_PUSHBUTTON);
            return result;
        }
        static std::vector<AccessibleItem> sections(const std::vector<std::wstring> &names, int selected,
                                                    int width, float scale) {
            std::vector<AccessibleItem> result;
            for (int sectionIndex = 0; sectionIndex < int(names.size()); ++sectionIndex) {
                result.push_back(
                    {section + sectionIndex, names[sectionIndex], L"Open this settings section.", L"Open",
                     EffectsLayout::Box{10, 64 + sectionIndex * 40, width - 20, 36}.pixels(scale),
                     ROLE_SYSTEM_LISTITEM,
                     STATE_SYSTEM_FOCUSABLE | STATE_SYSTEM_SELECTABLE |
                         (sectionIndex == selected ? STATE_SYSTEM_SELECTED : 0)});
            }
            return result;
        }
    };
}
