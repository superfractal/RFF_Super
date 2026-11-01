//
// Created by Merutilm on 2025-05-09.
// Modified by GPT-6 on 2026-09-23
// Modified by Opus 5.5 on 2026-10-25
//

#include "ParallelRenderState.h"

#include <mutex>

namespace merutilm::rff2 {

    bool ParallelRenderState::interruptRequested() const {
        if (runningOwner == this) {
            return runningToken->stop_requested();
        }
        return interrupted.load(std::memory_order_acquire);
    }

    void ParallelRenderState::interrupt() {
        interrupted.store(true, std::memory_order_release);
        std::scoped_lock lock(mutex);
        thread.request_stop();
    }

    void ParallelRenderState::cancel() {
        std::scoped_lock lock(mutex);
        cancelUnsafe();
    }

    void ParallelRenderState::cancelUnsafe() {
        if (thread.joinable()) {
            interrupted.store(true, std::memory_order_release);
            thread.request_stop();
            thread.join();
        }
    }
}
