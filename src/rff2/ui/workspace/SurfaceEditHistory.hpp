//
// Modified by GPT-6 on 2026-09-13, 2026-09-14, 2026-09-16, 2026-09-17, 2026-09-19, 2026-09-20, 2026-09-21, 2026-09-23, 2026-09-24
//

#pragma once
#include "WorkspaceText.hpp"
#include "HistoryOrder.hpp"
#include "SurfaceParameterRegistry.hpp"
#include "SurfaceColorRegistry.hpp"
#include "SurfaceEffectState.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace merutilm::rff2::workspace {
    class SurfaceEditHistory {
        static constexpr size_t maximumUndoEntries = 256;
        struct Edit {
            ShdSlopeAttribute before;
            ShdSlopeAttribute after;
            std::wstring label;
            uint64_t serial = 0;
        };
        std::vector<Edit> undoEntries;
        std::vector<Edit> redoEntries;
        std::optional<Edit> pending;
        bool hasChanges = false;
        HistoryOrder order;
        std::function<void(const ShdSlopeAttribute &, std::wstring)> editStarted;
        std::function<void(bool)> editFinished;
        void markChanged() {
            if (!hasChanges && pending && editStarted) {
                editStarted(pending->before, pending->label);
            }
            hasChanges = true;
        }

        static bool applyDelta(ShdSlopeAttribute &current, const ShdSlopeAttribute &from,
                               const ShdSlopeAttribute &to) {
            if (current.surfaceStyle != from.surfaceStyle) {
                return false;
            }
            for (const auto &parameter : surfaceParameters()) {
                if (parameter.value(from) != parameter.value(to) &&
                    parameter.value(current) != parameter.value(from)) {
                    return false;
                }
            }
            for (const auto &color : surfaceColors()) {
                if (from.*color.member != to.*color.member && current.*color.member != from.*color.member) {
                    return false;
                }
            }
            if (from.replaceSurfaceStyle != to.replaceSurfaceStyle &&
                current.replaceSurfaceStyle != from.replaceSurfaceStyle) {
                return false;
            }
            if (from.studio.use != to.studio.use && current.studio.use != from.studio.use) {
                return false;
            }
            if (from.layerCommon != to.layerCommon && current.layerCommon != from.layerCommon) {
                return false;
            }
            for (const auto &parameter : surfaceParameters()) {
                if (parameter.value(from) != parameter.value(to)) {
                    parameter.value(current) = parameter.value(to);
                }
            }
            for (const auto &color : surfaceColors()) {
                if (from.*color.member != to.*color.member) {
                    current.*color.member = to.*color.member;
                }
            }
            if (from.studio.use != to.studio.use) {
                current.studio.use = to.studio.use;
            }
            if (from.layerCommon != to.layerCommon) {
                current.layerCommon = to.layerCommon;
            }
            if (from.replaceSurfaceStyle != to.replaceSurfaceStyle) {
                current.replaceSurfaceStyle = to.replaceSurfaceStyle;
            }
            current.surfaceStyle = to.surfaceStyle;
            return true;
        }

      public:
        bool applyState(ShdSlopeAttribute &slope, const ShdSlopeAttribute &next, std::wstring label) {
            if (pending) {
                return false;
            }
            bool hasDifference = slope.replaceSurfaceStyle != next.replaceSurfaceStyle ||
                                 slope.surfaceStyle != next.surfaceStyle ||
                                 slope.studio.use != next.studio.use || slope.layerCommon != next.layerCommon;
            for (const auto &parameter : surfaceParameters()) {
                hasDifference |= parameter.value(slope) != parameter.value(next);
            }
            for (const auto &color : surfaceColors()) {
                hasDifference |= slope.*color.member != next.*color.member;
            }
            if (!hasDifference) {
                return false;
            }
            begin(slope, std::move(label));
            slope = next;
            pending->after = slope;
            markChanged();
            commit(slope);
            return true;
        }
        void observeEdits(std::function<void(const ShdSlopeAttribute &, std::wstring)> started,
                          std::function<void(bool)> finished) {
            editStarted = std::move(started);
            editFinished = std::move(finished);
        }
        bool resetValue(ShdSlopeAttribute &slope, const SurfaceParameter &parameter) {
            const auto defaults = surfaceDefaults();
            const float value = parameter.value(defaults);
            if (pending || !parameter.valid(value) || parameter.value(slope) == value) {
                return false;
            }
            begin(slope, L"Reset " + std::wstring(parameter.label));
            parameter.value(slope) = value;
            markChanged();
            commit(slope);
            return true;
        }
        bool resetValue(ShdSlopeAttribute &slope, const SurfaceColor &color) {
            const auto defaults = surfaceDefaults();
            const auto value = defaults.*color.member;
            if (pending || !color.valid(value) || slope.*color.member == value) {
                return false;
            }
            begin(slope, L"Reset " + std::wstring(color.label));
            slope.*color.member = value;
            markChanged();
            commit(slope);
            return true;
        }
        static bool hasAddedEffects(const ShdSlopeAttribute &slope) {
            return slope.layerCommon != 0 || slope.layerMetal != 0 || slope.layerSigil != 0 ||
                   slope.layerPhonk != 0 || slope.layerFrost != 0 || slope.layerSea != 0 ||
                   slope.layerPrint != 0 || slope.layerQuantize != 0 || slope.layerVhs != 0 ||
                   slope.layerMono != 0;
        }
        bool clearAddedEffects(ShdSlopeAttribute &slope) {
            if (pending || !hasAddedEffects(slope)) {
                return false;
            }
            begin(slope, L"Clear added surface effects");
            slope.layerCommon = 0;
            slope.layerMetal = 0;
            slope.layerSigil = 0;
            slope.layerPhonk = 0;
            slope.layerFrost = 0;
            slope.layerSea = 0;
            slope.layerPrint = 0;
            slope.layerQuantize = 0;
            slope.layerVhs = 0;
            slope.layerMono = 0;
            markChanged();
            commit(slope);
            return true;
        }
        bool clearCategoryEffects(ShdSlopeAttribute &slope, Category category) {
            if (pending || !effectState(slope, category).added) {
                return false;
            }
            begin(slope, L"Remove added effects");
            switch (category) {
            case Category::COLOR:
                slope.layerCommon = 0;
                break;
            case Category::REFLECTION:
            case Category::FILM:
                slope.layerMetal = 0;
                break;
            case Category::CONTOUR:
                slope.layerSigil = 0;
                break;
            case Category::EMISSION:
                slope.layerSea = 0;
                break;
            case Category::FLAME:
                slope.layerPhonk = 0;
                slope.layerFrost = 0;
                break;
            case Category::PRINT:
                slope.layerPrint = 0;
                slope.layerQuantize = 0;
                break;
            case Category::NOISE:
                slope.layerVhs = 0;
                slope.layerMono = 0;
                break;
            case Category::MIX:
                slope.layerCommon = 0;
                slope.layerMetal = 0;
                slope.layerSigil = 0;
                slope.layerPhonk = 0;
                slope.layerFrost = 0;
                slope.layerSea = 0;
                slope.layerPrint = 0;
                break;
            case Category::RELIEF:
                break;
            }
            markChanged();
            commit(slope);
            return true;
        }
        static bool canResetGroup(const ShdSlopeAttribute &slope, Category category) {
            return changedSurfaceControls(slope, category) > 0;
        }
        bool resetGroup(ShdSlopeAttribute &slope, Category category) {
            if (pending || !canResetGroup(slope, category)) {
                return false;
            }
            const ShdSlopeAttribute defaults = surfaceDefaults();
            for (const auto &parameter : surfaceParameters()) {
                if (parameter.category() == category && !parameter.valid(parameter.value(defaults))) {
                    return false;
                }
            }
            begin(slope, L"Reset effect controls");
            for (const auto &parameter : surfaceParameters()) {
                if (!paletteLineControl(parameter.id) && parameter.category() == category) {
                    parameter.value(slope) = parameter.value(defaults);
                }
            }
            for (const auto &color : surfaceColors()) {
                if (!paletteLineControl(color.id) && color.category == category) {
                    slope.*color.member = defaults.*color.member;
                }
            }
            pending->after = slope;
            markChanged();
            commit(slope);
            return true;
        }
        void begin(const ShdSlopeAttribute &slope, std::wstring label) {
            if (pending) {
                return;
            }
            pending = Edit{slope, slope, std::move(label)};
            hasChanges = false;
        }
        bool setColor(ShdSlopeAttribute &slope, const SurfaceColor &parameter, const glm::vec4 &color) {
            if (pending || !parameter.valid(color) || slope.*parameter.member == color) {
                return false;
            }
            begin(slope, std::wstring(parameter.label));
            slope.*parameter.member = color;
            activateSurfaceColor(slope, &(slope.*parameter.member));
            pending->after = slope;
            markChanged();
            commit(slope);
            return true;
        }
        bool setStyle(ShdSlopeAttribute &slope, ShdSurfaceStyle style) {
            if (static_cast<int>(style) < 0 || static_cast<int>(style) > 6 || slope.surfaceStyle == style) {
                return false;
            }
            if (pending) {
                return false;
            }
            begin(slope, uiText(TextKey::BaseStyle));
            slope.surfaceStyle = style;
            if (style != ShdSurfaceStyle::ORIGINAL) {
                slope.studio.use = true;
            }
            pending->after = slope;
            markChanged();
            commit(slope);
            return true;
        }
        bool setStudio(ShdSlopeAttribute &slope, bool enabled) {
            if (pending || slope.studio.use == enabled) {
                return false;
            }
            begin(slope, L"Studio");
            slope.studio.use = enabled;
            pending->after = slope;
            markChanged();
            commit(slope);
            return true;
        }
        bool set(ShdSlopeAttribute &slope, const SurfaceParameter &parameter, float value) {
            if (!parameter.valid(value) || parameter.value(slope) == value) {
                return false;
            }
            const bool commitImmediately = !pending;
            if (commitImmediately) {
                begin(slope, std::wstring(parameter.label));
            }
            parameter.value(slope) = value;
            parameter.activate(slope);
            if (parameter.category() == Category::MIX) {
                slope.studio.use = true;
            }
            pending->after = slope;
            markChanged();
            if (commitImmediately) {
                commit(slope);
            }
            return true;
        }
        void commit(const ShdSlopeAttribute &slope) {
            if (!pending) {
                return;
            }
            if (hasChanges) {
                pending->after = slope;
                pending->serial = order.commit();
                undoEntries.push_back(std::move(*pending));
                if (undoEntries.size() > maximumUndoEntries) {
                    undoEntries.erase(undoEntries.begin());
                }
                redoEntries.clear();
                if (editFinished) {
                    editFinished(true);
                }
            }
            pending.reset();
            hasChanges = false;
        }
        void cancel(ShdSlopeAttribute &slope) {
            if (pending) {
                applyDelta(slope, pending->after, pending->before);
            }
            if (hasChanges && editFinished) {
                editFinished(false);
            }
            pending.reset();
            hasChanges = false;
        }
        bool undo(ShdSlopeAttribute &slope) {
            if (pending || undoEntries.empty()) {
                return false;
            }
            if (!applyDelta(slope, undoEntries.back().after, undoEntries.back().before)) {
                clear();
                return false;
            }
            order.prepareUndo(redoEntries);
            redoEntries.push_back(std::move(undoEntries.back()));
            undoEntries.pop_back();
            return true;
        }
        bool redo(ShdSlopeAttribute &slope) {
            if (!canRedo()) {
                return false;
            }
            if (!applyDelta(slope, redoEntries.back().before, redoEntries.back().after)) {
                clear();
                return false;
            }
            undoEntries.push_back(std::move(redoEntries.back()));
            redoEntries.pop_back();
            return true;
        }
        void clear() {
            if (hasChanges && editFinished) {
                editFinished(false);
            }
            pending.reset();
            hasChanges = false;
            undoEntries.clear();
            redoEntries.clear();
        }
        bool canUndo() const {
            return !pending && !undoEntries.empty();
        }
        bool canRedo() const {
            return !pending && order.validRedo() && !redoEntries.empty();
        }
        void bindHistory(std::shared_ptr<HistoryDomain> domain) {
            clear();
            order.bind(std::move(domain));
        }
        uint64_t undoOrder() const {
            return canUndo() ? undoEntries.back().serial : 0;
        }
        uint64_t redoOrder() const {
            return canRedo() ? redoEntries.back().serial : 0;
        }
    };
} // namespace merutilm::rff2::workspace
