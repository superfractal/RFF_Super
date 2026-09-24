//
// Created by Merutilm on 2025-07-21.
// Modified by GPT-6 on 2026-09-22, 2026-09-23
//

#include "ScopedNewCommandBufferExecutor.hpp"

#include <exception>

#include "../core/vkh_core.hpp"
#include "../impl/Fence.hpp"

namespace merutilm::vkh {
    ScopedNewCommandBufferExecutor::ScopedNewCommandBufferExecutor(CoreRef core, CommandPoolRef commandPool,
                                                                   FencePtr const fence) : CoreHandler(core),
        commandPool(commandPool), fence(fence) {
        ScopedNewCommandBufferExecutor::init();
    }

    ScopedNewCommandBufferExecutor::~ScopedNewCommandBufferExecutor() {
        ScopedNewCommandBufferExecutor::destroy();
    }

    VkResult ScopedNewCommandBufferExecutor::waitForCompletion() const {
        const VkFence fenceHandle = fence == nullptr ? ownedFence : fence->getFenceHandle();
        return vkWaitForFences(core.getLogicalDevice().getLogicalDeviceHandle(), 1, &fenceHandle, VK_TRUE, UINT64_MAX);
    }

    void ScopedNewCommandBufferExecutor::init() {
        if (const VkCommandBufferAllocateInfo allocInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext = nullptr,
                .commandPool = commandPool.getCommandPoolHandle(),
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1
            };
            allocator::invoke(vkAllocateCommandBuffers, core.getLogicalDevice().getLogicalDeviceHandle(), &allocInfo,
                                     &commandBuffer) != VK_SUCCESS) {
            throw exception_init("Failed to allocate command buffers!");
        }

        constexpr VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pInheritanceInfo = nullptr
        };
        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            allocator::invoke(vkFreeCommandBuffers, core.getLogicalDevice().getLogicalDeviceHandle(),
                              commandPool.getCommandPoolHandle(), 1, &commandBuffer);
            throw exception_init("Failed to begin command buffer operation.");
        }
    }

    void ScopedNewCommandBufferExecutor::finish() {
        if (commandBuffer == VK_NULL_HANDLE) {
            throw exception_invalid_state("Command buffer has already been finished.");
        }
        if (const VkResult result = vkEndCommandBuffer(commandBuffer); result != VK_SUCCESS) {
            throw exception_invalid_state(std::string("Failed to end command buffer! ") + string_VkResult(result));
        }
        const VkDevice device = core.getLogicalDevice().getLogicalDeviceHandle();
        if (fence == nullptr) {
            constexpr VkFenceCreateInfo fenceInfo = {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0
            };
            VkFence createdFence = VK_NULL_HANDLE;
            if (const VkResult result = allocator::invoke(vkCreateFence, device, &fenceInfo, nullptr, &createdFence);
                result != VK_SUCCESS) {
                throw exception_init(std::string("Failed to create transfer fence! ") + string_VkResult(result));
            }
            ownedFence = createdFence;
        }
        const VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = nullptr,
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffer,
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr
        };

        const VkResult submitResult = core.getLogicalDevice().queueSubmitResult(
            1, &submitInfo, fence == nullptr ? ownedFence : fence->getFenceHandle());
        if (submitResult != VK_SUCCESS && submitResult != VK_ERROR_DEVICE_LOST) {
            throw exception_invalid_state(std::string("Failed to submit queue! ") + string_VkResult(submitResult));
        }
        // Device loss may leave the submission pending until a wait confirms its lifetime state.
        submitted = true;
        VkResult waitResult = waitForCompletion();
        if (waitResult != VK_SUCCESS && waitResult != VK_ERROR_DEVICE_LOST) {
            waitResult = core.getLogicalDevice().waitDeviceIdle();
            if (waitResult != VK_SUCCESS && waitResult != VK_ERROR_DEVICE_LOST) {
                // An unconfirmed submission cannot release its command buffer or recorded resources.
                std::terminate();
            }
        }
        submitted = false;
        const VkResult idleResult = fence == nullptr && submitResult == VK_SUCCESS && waitResult == VK_SUCCESS
                                        ? core.getLogicalDevice().waitDeviceIdle()
                                        : VK_SUCCESS;
        allocator::invoke(vkFreeCommandBuffers, device, commandPool.getCommandPoolHandle(), 1, &commandBuffer);
        commandBuffer = VK_NULL_HANDLE;
        if (ownedFence != VK_NULL_HANDLE) {
            allocator::invoke(vkDestroyFence, device, ownedFence, nullptr);
            ownedFence = VK_NULL_HANDLE;
        }
        if (submitResult != VK_SUCCESS) {
            throw exception_invalid_state(std::string("Failed to submit queue! ") + string_VkResult(submitResult));
        }
        if (waitResult != VK_SUCCESS) {
            throw exception_invalid_state(std::string("Failed to wait for command buffer! ") + string_VkResult(waitResult));
        }
        if (idleResult != VK_SUCCESS) {
            throw exception_invalid_state(std::string("Failed to wait for device! ") + string_VkResult(idleResult));
        }
    }

    void ScopedNewCommandBufferExecutor::destroy() noexcept {
        if (commandBuffer == VK_NULL_HANDLE) {
            return;
        }
        const VkDevice device = core.getLogicalDevice().getLogicalDeviceHandle();
        if (submitted) {
            VkResult waitResult = waitForCompletion();
            if (waitResult != VK_SUCCESS && waitResult != VK_ERROR_DEVICE_LOST) {
                waitResult = core.getLogicalDevice().waitDeviceIdle();
                if (waitResult != VK_SUCCESS && waitResult != VK_ERROR_DEVICE_LOST) {
                    std::terminate();
                }
            }
        }
        allocator::invoke(vkFreeCommandBuffers, device, commandPool.getCommandPoolHandle(), 1, &commandBuffer);
        commandBuffer = VK_NULL_HANDLE;
        if (ownedFence != VK_NULL_HANDLE) {
            allocator::invoke(vkDestroyFence, device, ownedFence, nullptr);
            ownedFence = VK_NULL_HANDLE;
        }
    }
}
