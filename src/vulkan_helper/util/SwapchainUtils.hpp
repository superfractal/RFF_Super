//
// Created by Merutilm on 2025-07-26.
// Modified by GPT-5 on 2026-08-23.
//

#pragma once
#include "../core/vkh.hpp"

namespace merutilm::vkh {
    struct SwapchainUtils {
        struct AcquiredImage {
            uint32_t index;
            bool recreate;
        };

        SwapchainUtils() = delete;


        template <typename F> requires std::is_invocable_r_v<void, F, uint32_t>
        static bool renderFrame(WindowContextRef wc, uint32_t *frameIndex, F&&renderer) {
            if (wc.getWindow().isUnrenderable()) {
                return false;
            }
            changeFrameIndex(wc.core, frameIndex);
            const auto acquired = begin(wc, *frameIndex);
            if (!acquired.has_value()) {
                return true;
            }
            renderer(acquired->index);
            return end(wc, *frameIndex, acquired->index) || acquired->recreate;
        }

        static void changeFrameIndex(CoreRef core, uint32_t *frameIndex) {
            ++*frameIndex %= core.getPhysicalDevice().getMaxFramesInFlight();
        }

        static std::optional<AcquiredImage> begin(WindowContextRef wc, const uint32_t frameIndex) {
            const SwapchainRef swapchain = wc.getSwapchain();
            const VkDevice device = wc.core.getLogicalDevice().getLogicalDeviceHandle();
            const VkSemaphore imageAvailableSemaphore = wc.getSyncObject().
                    getSemaphore(frameIndex).getImageAvailable();
            const VkSwapchainKHR swapchainHandle = swapchain.getSwapchainHandle();


            wc.getSyncObject().getFence(frameIndex).wait();

            uint32_t swapchainImageIndex = 0;
            const VkResult result = allocator::invoke(vkAcquireNextImageKHR, device, swapchainHandle, UINT64_MAX,
                                                      imageAvailableSemaphore, nullptr, &swapchainImageIndex);
            if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                return std::nullopt;
            }
            if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                throw exception_invalid_state(std::string("Failed to acquire swapchain image! ") +
                                              string_VkResult(result));
            }
            wc.getSyncObject().getFence(frameIndex).reset();
            return AcquiredImage{swapchainImageIndex, result == VK_SUBOPTIMAL_KHR};
        }

        static bool end(WindowContextRef wc, const uint32_t frameIndex, uint32_t swapchainImageIndex) {
            VkSwapchainKHR swapchainHandle = wc.getSwapchain().getSwapchainHandle();
            VkSemaphore renderFinishedSemaphore = wc.getSyncObject().getSemaphore(frameIndex).getRenderFinished();
            const VkPresentInfoKHR presentInfo = {
                .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                .pNext = nullptr,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &renderFinishedSemaphore,
                .swapchainCount = 1,
                .pSwapchains = &swapchainHandle,
                .pImageIndices = &swapchainImageIndex,
                .pResults = nullptr
            };
            const VkResult result = wc.core.getLogicalDevice().queuePresent(&presentInfo);
            return result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR;
        }
    };
}
