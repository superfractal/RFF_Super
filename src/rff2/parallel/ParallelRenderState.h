//
// Created by Merutilm on 2025-05-09.
// Modified by GPT-5 on 2026-09-01
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include <atomic>
#include <exception>
#include <functional>
#include <mutex>
#include <stop_token>
#include <thread>
#include <type_traits>
#include <utility>

#include "../../vulkan_helper/core/logger.hpp"

namespace merutilm::rff2 {
    class ParallelRenderState final {
        mutable std::mutex mutex;
        std::jthread thread = std::jthread([](const std::stop_token&) {
            //default empty thread
        });
        std::atomic<bool> interrupted{false};
        inline static thread_local const ParallelRenderState *runningOwner = nullptr;
        inline static thread_local const std::stop_token *runningToken = nullptr;

    public:
        ParallelRenderState() = default;

        template<typename T>
            requires std::is_invocable_r_v<void, T, const std::stop_token&>
        void createThread(T&& func);

        [[nodiscard]] std::stop_token stopToken() const;

        [[nodiscard]] bool interruptRequested() const;

        void cancel();

        void interrupt();

    private:
        void cancelUnsafe();
    };

    template<typename T>
        requires std::is_invocable_r_v<void, T, const std::stop_token&>
    void ParallelRenderState::createThread(T&& func) {
        std::scoped_lock lock(mutex);
        cancelUnsafe();
        interrupted.store(false, std::memory_order_release);
        thread = std::jthread([this, f = std::forward<T>(func)](const std::stop_token& interrupted) mutable {
            const auto *previousOwner = runningOwner;
            const auto *previousToken = runningToken;
            runningOwner = this;
            runningToken = &interrupted;
            try {
                f(interrupted);
            } catch (const std::exception& error) {
                try {
                    vkh::logger::log_err_silent("Render worker failed: {}", error.what());
                } catch (...) {
                }
            } catch (...) {
                try {
                    vkh::logger::log_err_silent("Render worker failed with an unknown exception");
                } catch (...) {
                }
            }
            runningOwner = previousOwner;
            runningToken = previousToken;
        });
    }
}
