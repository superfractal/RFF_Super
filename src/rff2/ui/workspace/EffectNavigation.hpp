//
// Modified by GPT-6 on 2026-09-13, 2026-09-14, 2026-09-18, 2026-09-19, 2026-09-22
//

#pragma once
#include "SurfaceEffectState.hpp"
#include <bitset>
#include <array>

namespace merutilm::rff2::workspace {
    enum class EffectFilter { ALL, USED, FAVORITES, CHANGED, RECENT, COUNT };

    class EffectNavigation {
        std::bitset<10> favorites;
        std::vector<Category> recent;

      public:
        static constexpr int filterCount = 3;
        static constexpr size_t recentLimit = 5;
        static constexpr std::array<std::wstring_view, filterCount> filterLabels = {L"All", L"Used",
                                                                                    L"Favorites"};
        static constexpr std::array<std::wstring_view, filterCount> filterHelp = {
            L"Show all surface effect categories.", L"Show effects used by the base style or added layers.",
            L"Show favorite effects. Favorites are workspace preferences, not artwork settings."};
        EffectFilter filter = EffectFilter::ALL;

        bool favorite(Category category) const {
            return favorites.test(static_cast<size_t>(category));
        }
        void toggleFavorite(Category category) {
            favorites.flip(static_cast<size_t>(category));
        }
        const std::vector<Category> &recentCategories() const {
            return recent;
        }
        bool remember(Category category) {
            if (int(category) < 0 || int(category) >= 10) {
                return false;
            }
            if (!recent.empty() && recent.front() == category) {
                return false;
            }
            std::erase(recent, category);
            recent.insert(recent.begin(), category);
            if (recent.size() > recentLimit) {
                recent.pop_back();
            }
            return true;
        }
        std::wstring stateLabel(const ShdSlopeAttribute &slope, Category category) const {
            if (filter == EffectFilter::CHANGED) {
                const int count = changedSurfaceControls(slope, category);
                return std::to_wstring(count) + (count == 1 ? L" non-default value" : L" non-default values");
            }
            const auto state = effectState(slope, category);
            if (state.base || state.added) {
                return std::wstring(state.label());
            }
            return L"";
        }
        std::array<std::wstring_view, 2> emptyText() const {
            switch (filter) {
            case EffectFilter::USED:
                return {L"No effects in use", L"Choose a base style."};
            case EffectFilter::FAVORITES:
                return {L"No favorite effects", L"Use a star in All."};
            case EffectFilter::CHANGED:
                return {L"No changed effects", L"Values match defaults."};
            case EffectFilter::RECENT:
                return {L"Nothing opened yet", L"Open an effect in All."};
            default:
                return {L"No effects in this view", L"Choose All to reset."};
            }
        }

        std::vector<Category> rows(const ShdSlopeAttribute &slope) const {
            std::vector<Category> result;
            for (int i = 0; i < 10; ++i) {
                const auto category = static_cast<Category>(i);
                if (category == Category::CONTOUR || category == Category::RELIEF) {
                    continue;
                }
                const auto state = effectState(slope, category);
                const bool used = state.studio && (state.base || state.added);
                if (filter == EffectFilter::ALL || (filter == EffectFilter::USED && used) ||
                    (filter == EffectFilter::FAVORITES && favorite(category)) ||
                    (filter == EffectFilter::CHANGED && changedSurfaceControls(slope, category) > 0) ||
                    (filter == EffectFilter::RECENT &&
                     std::find(recent.begin(), recent.end(), category) != recent.end())) {
                    result.push_back(category);
                }
            }
            return result;
        }

        Category move(const ShdSlopeAttribute &slope, Category current, int direction) const {
            const auto categories = rows(slope);
            if (categories.empty()) {
                return current;
            }
            const auto found = std::find(categories.begin(), categories.end(), current);
            if (found == categories.end()) {
                if (direction < 0) {
                    return categories.back();
                }
                return categories.front();
            }
            const int index = static_cast<int>(found - categories.begin());
            return categories[std::clamp(index + direction, 0, static_cast<int>(categories.size()) - 1)];
        }
    };
}
