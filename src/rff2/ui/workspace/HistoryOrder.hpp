//
// Modified by GPT-6 on 2026-09-14, 2026-09-21
//

#pragma once
#include <cstdint>
#include <memory>

namespace merutilm::rff2::workspace {
    struct HistoryDomain {
        uint64_t lastCommit = 0;
    };
    class HistoryOrder {
        std::shared_ptr<HistoryDomain> domain = std::make_shared<HistoryDomain>();
        uint64_t redoGeneration = 0;

      public:
        void bind(std::shared_ptr<HistoryDomain> sharedDomain) {
            domain = std::move(sharedDomain);
            redoGeneration = domain->lastCommit;
        }
        uint64_t commit() {
            return ++domain->lastCommit;
        }
        bool latest(uint64_t serial) const {
            return serial == domain->lastCommit;
        }
        bool validRedo() const {
            return redoGeneration == domain->lastCommit;
        }
        template <class Stack> void prepareUndo(Stack &redoEntries) {
            if (!validRedo()) {
                redoEntries.clear();
            }
            redoGeneration = domain->lastCommit;
        }
    };
} // namespace merutilm::rff2::workspace
