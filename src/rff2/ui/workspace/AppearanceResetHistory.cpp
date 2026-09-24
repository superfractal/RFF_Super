//
// Modified by GPT-6 on 2026-09-14, 2026-09-19, 2026-09-21
//

#include "AppearanceResetHistory.hpp"
#include "../../io/ShaderPresetIO.h"

namespace merutilm::rff2::workspace {
    bool AppearanceResetHistory::canReset() const {
        return AppearanceState::key(attribute()) != AppearanceState::key(defaults());
    }
    bool AppearanceResetHistory::reset() {
        const auto before = attribute();
        auto after = defaults();
        AppearanceState::keepMotion(before, after);
        return apply(std::move(after));
    }
    bool AppearanceResetHistory::apply(ShaderAttribute after) {
        const auto before = attribute();
        if (!ShaderPresetIO::validate(after) || AppearanceState::key(before) == AppearanceState::key(after)) {
            return false;
        }
        const auto serial = order.commit();
        undoEntries.push_back({before, after, serial});
        if (undoEntries.size() > 64) {
            undoEntries.erase(undoEntries.begin());
        }
        redoEntries.clear();
        attribute() = std::move(after);
        return true;
    }
    bool AppearanceResetHistory::restore(bool redo) {
        auto &sourceHistory = redo ? redoEntries : undoEntries;
        auto &destinationHistory = redo ? undoEntries : redoEntries;
        if (sourceHistory.empty() || (redo && !order.validRedo())) {
            return false;
        }
        const auto &expected = redo ? sourceHistory.back().before : sourceHistory.back().after;
        if (AppearanceState::key(attribute()) != AppearanceState::key(expected)) {
            clear();
            return false;
        }
        auto target = redo ? sourceHistory.back().after : sourceHistory.back().before;
        AppearanceState::keepMotion(attribute(), target);
        if (!redo) {
            order.prepareUndo(redoEntries);
        }
        destinationHistory.push_back(std::move(sourceHistory.back()));
        sourceHistory.pop_back();
        attribute() = std::move(target);
        return true;
    }
} // namespace merutilm::rff2::workspace
