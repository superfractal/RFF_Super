//
// Created by Merutilm on 2025-08-28.
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include "../handle/WindowContextHandler.hpp"
#include "../impl/Engine.hpp"

namespace merutilm::vkh {
    class ScopedCommandBufferExecutor final : public WindowContextHandler {
        const uint32_t frameIndex;
        const VkSemaphore imageAvailable;
        const VkSemaphore renderFinished;
        bool finished = false;
    public:
        explicit ScopedCommandBufferExecutor(WindowContextRef wc, uint32_t frameIndex, VkSemaphore imageAvailable, VkSemaphore renderFinished);

        ~ScopedCommandBufferExecutor() override;

        ScopedCommandBufferExecutor(const ScopedCommandBufferExecutor &) = delete;

        ScopedCommandBufferExecutor &operator=(const ScopedCommandBufferExecutor &) = delete;

        ScopedCommandBufferExecutor(ScopedCommandBufferExecutor &&) = delete;

        ScopedCommandBufferExecutor &operator=(ScopedCommandBufferExecutor &&) = delete;

        void init() override;

        void destroy() override;

        void finish();
    };
}
