//
// Created by Merutilm on 2025-06-09.
// Modified by Opus 5 on 2026-08-31
// Modified by GPT-6 on 2026-09-22, 2026-09-23
//

#pragma once
#include <mutex>
#include "BackgroundThread.h"

namespace merutilm::rff2 {
    class BackgroundThreads final {
        mutable std::mutex mutex;
        std::vector<std::unique_ptr<BackgroundThread>> threads;

    public:

        template<typename T> requires std::is_invocable_r_v<void, T, BackgroundThread &>
        void createThread(T &&func) {
            std::scoped_lock lock(mutex);

            uint32_t index = threads.size();
            for (uint32_t i = 0; i < threads.size(); ++i) {
                if (threads[i]->isFinished()) {
                    threads[i]->tryJoin();
                    index = i;
                    break;
                }
            }
            const bool appendedSlot = index == threads.size();
            if (appendedSlot) {
                threads.emplace_back(nullptr);
            }
            try {
                threads[index] = std::make_unique<BackgroundThread>(std::forward<T>(func));
            } catch (...) {
                if (appendedSlot) {
                    threads.pop_back();
                }
                throw;
            }
        }

        void notifyAll() {
            std::scoped_lock lock(mutex);
            for (const auto &thread : threads) {
                thread->notify();
            }
        }

        void requestStopAll() {
            std::scoped_lock lock(mutex);
            for (const auto &thread : threads) {
                if (thread != nullptr) {
                    thread->requestStop();
                }
            }
        }

        // How many of the slots hold a worker that has not run out yet. Finished ones are kept until
        // the next createThread reuses the slot, so counting the vector alone would over-report.
        [[nodiscard]] size_t runningCount() const {
            std::scoped_lock lock(mutex);
            size_t running = 0;
            for (const auto &thread : threads) {
                if (thread != nullptr && !thread->isFinished()) {
                    ++running;
                }
            }
            return running;
        }
    };




}
