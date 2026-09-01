//
// Created by Merutilm on 2025-05-09.
// Modified by GPT-5 on 2026-09-01
//

#pragma once
#include <exception>
#include <functional>
#include <mutex>
#include <thread>

#include "../../vulkan_helper/core/logger.hpp"

namespace merutilm::rff2 {
    class ParallelRenderState final {
        std::mutex mutex;
        std::jthread thread = std::jthread([](const std::stop_token&) {
            //default empty thread
        });


    public:
        ParallelRenderState() = default;

        template<typename T> requires std::is_invocable_r_v<void, T, const std::stop_token &>
        void createThread(T &&func);

        [[nodiscard]] std::stop_token stopToken() const;

        [[nodiscard]] bool interruptRequested() const;

        void cancel();

        void interrupt();

    private:
        void cancelUnsafe();
    };

    template<typename T> requires std::is_invocable_r_v<void, T, const std::stop_token &>
    void ParallelRenderState::createThread(T &&func) {
        std::scoped_lock lock(mutex);
        cancelUnsafe();
        thread = std::jthread([f = std::forward<T>(func)](const std::stop_token &interrupted) mutable {
            try {
                f(interrupted);
            } catch (const std::exception &error) {
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
        });
    }
}
