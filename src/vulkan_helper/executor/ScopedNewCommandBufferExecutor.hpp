//
// Created by Merutilm on 2025-07-21.
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include "../handle/CoreHandler.hpp"
#include "../impl/CommandPool.hpp"
#include "../impl/Fence.hpp"

namespace merutilm::vkh {
    class ScopedNewCommandBufferExecutor final : public CoreHandler {
        CommandPoolRef commandPool;
        FencePtr const fence;
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        VkFence ownedFence = VK_NULL_HANDLE;
        bool submitted = false;

    public:
        explicit ScopedNewCommandBufferExecutor(CoreRef core, CommandPoolRef commandPool, FencePtr fence = VK_NULL_HANDLE);

        ~ScopedNewCommandBufferExecutor() override;

        ScopedNewCommandBufferExecutor(const ScopedNewCommandBufferExecutor &) = delete;

        ScopedNewCommandBufferExecutor &operator=(const ScopedNewCommandBufferExecutor &) = delete;

        ScopedNewCommandBufferExecutor(ScopedNewCommandBufferExecutor &&) = delete;

        ScopedNewCommandBufferExecutor &operator=(ScopedNewCommandBufferExecutor &&) = delete;

        [[nodiscard]] VkCommandBuffer getCommandBufferHandle() const { return commandBuffer; }

        void finish();

    private:
        [[nodiscard]] VkResult waitForCompletion() const;

        void init() override;

        void destroy() noexcept override;
    };
}
