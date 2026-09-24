//
// Modified by GPT-6 on 2026-09-14, 2026-09-19, 2026-09-21
//

#pragma once
#include "HistoryOrder.hpp"
#include "AppearanceState.hpp"
#include "../../attr/ShaderAttribute.h"
#include <functional>
#include <string>
#include <vector>

namespace merutilm::rff2::workspace {
    class AppearanceResetHistory {
        struct Edit {
            ShaderAttribute before, after;
            uint64_t serial;
        };
        HistoryOrder order;
        std::vector<Edit> undoEntries, redoEntries;
        std::function<ShaderAttribute &()> attribute;
        bool restore(bool redo);

      public:
        explicit AppearanceResetHistory(std::function<ShaderAttribute &()> attribute)
            : attribute(std::move(attribute)) {}
        static ShaderAttribute defaults() {
            return AppearanceState::defaults();
        }
        bool reset();
        bool apply(ShaderAttribute after);
        bool canReset() const;
        bool undo() {
            return restore(false);
        }
        bool redo() {
            return restore(true);
        }
        void clear() {
            undoEntries.clear();
            redoEntries.clear();
        }
        void bindHistory(std::shared_ptr<HistoryDomain> domain) {
            clear();
            order.bind(std::move(domain));
        }
        uint64_t undoOrder() const {
            return undoEntries.empty() ? 0 : undoEntries.back().serial;
        }
        uint64_t redoOrder() const {
            return !order.validRedo() || redoEntries.empty() ? 0 : redoEntries.back().serial;
        }
    };
} // namespace merutilm::rff2::workspace
