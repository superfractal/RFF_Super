//
// Created by Merutilm on 2025-08-28.
// Modified by GPT-6 on 2026-09-23
//

#include "ScopedCommandBufferExecutor.hpp"

#include <exception>

#include "../core/logger.hpp"

namespace merutilm::vkh {
    ScopedCommandBufferExecutor::ScopedCommandBufferExecutor(
        WindowContextRef wc, const uint32_t frameIndex, const VkSemaphore imageAvailable,
        const VkSemaphore renderFinished) : WindowContextHandler(wc),
                                            frameIndex(frameIndex),
                                            imageAvailable(imageAvailable), renderFinished(renderFinished) {
        ScopedCommandBufferExecutor::init();
    }

    ScopedCommandBufferExecutor::~ScopedCommandBufferExecutor() {
        ScopedCommandBufferExecutor::destroy();
    }


    void ScopedCommandBufferExecutor::init() {
        const VkCommandBuffer cbh = wc.getCommandBuffer().getCommandBufferHandle(frameIndex);
        constexpr VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pInheritanceInfo = nullptr
        };

        if (const VkResult result = vkResetCommandBuffer(cbh, 0); result != VK_SUCCESS) {
            throw exception_init(std::string("Failed to reset command buffer! ") + string_VkResult(result));
        }
        if (vkBeginCommandBuffer(cbh, &beginInfo) != VK_SUCCESS) {
            throw exception_init("Failed to begin command buffer operation.");
        }
    }

    void ScopedCommandBufferExecutor::destroy() {
    }

    void ScopedCommandBufferExecutor::finish() {
        if (finished) {
            throw exception_invalid_state("Command buffer has already been submitted.");
        }
        const VkCommandBuffer cbh = wc.getCommandBuffer().getCommandBufferHandle(frameIndex);
        if (const VkResult result = vkEndCommandBuffer(cbh); result != VK_SUCCESS) {
            throw exception_invalid_state(std::string("Failed to end command buffer! ") + string_VkResult(result));
        }

        constexpr VkPipelineStageFlags waitPipelineStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        const VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = imageAvailable == VK_NULL_HANDLE ? 0 : 1u,
            .pWaitSemaphores = imageAvailable == VK_NULL_HANDLE ? nullptr : &imageAvailable,
            .pWaitDstStageMask = imageAvailable == VK_NULL_HANDLE ? nullptr : &waitPipelineStage,
            .commandBufferCount = 1u,
            .pCommandBuffers = &cbh,
            .signalSemaphoreCount = renderFinished == VK_NULL_HANDLE ? 0 : 1u,
            .pSignalSemaphores = renderFinished == VK_NULL_HANDLE ? nullptr : &renderFinished,
        };
        FenceRef frameFence = wc.getSyncObject().getFence(frameIndex);
        frameFence.reset();
        const VkFence fenceHandle = frameFence.getFenceHandle();
        const VkResult result = wc.core.getLogicalDevice().queueSubmitResult(
            1, &submitInfo, fenceHandle);
        if (result != VK_SUCCESS) {
            if (result == VK_ERROR_DEVICE_LOST) {
                // A lost-device submission may remain pending until a wait confirms its lifetime state.
                VkResult completion = vkWaitForFences(
                    wc.core.getLogicalDevice().getLogicalDeviceHandle(), 1, &fenceHandle, VK_TRUE, UINT64_MAX);
                if (completion != VK_SUCCESS && completion != VK_ERROR_DEVICE_LOST) {
                    completion = wc.core.getLogicalDevice().waitDeviceIdle();
                    if (completion != VK_SUCCESS && completion != VK_ERROR_DEVICE_LOST) {
                        std::terminate();
                    }
                }
            } else {
                frameFence.markUnsubmitted();
            }
            throw exception_invalid_state(std::string("Failed to submit queue! ") + string_VkResult(result));
        }
        frameFence.markSubmitted();
        finished = true;
    }
}
