//
// Created by Merutilm on 2025-07-26.
// Modified by Opus 5 on 2026-08-15, 2026-08-26
// Modified by GPT-5 on 2026-08-23.
//

#pragma once
#include "../core/vkh_core.hpp"
#include "../impl/Swapchain.hpp"
#include "../struct/ImageInitInfo.hpp"
#include "../util/BufferImageUtils.hpp"

namespace merutilm::vkh {
    struct ImageContext;

    using MultiframeImageContext = std::vector<ImageContext>;

    struct ImageContext {
        VkImage image = VK_NULL_HANDLE;
        VkFormat imageFormat = VK_FORMAT_UNDEFINED;
        VkDeviceMemory imageMemory = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkImageView mipmappedImageView = VK_NULL_HANDLE;
        VkExtent2D extent = {};
        VkDeviceSize capacity = 0;

        static ImageContext createContext(CoreRef core, const ImageInitInfo &imageInitInfo) {
            ImageContext result = {};
            BufferImageUtils::initImage(core, imageInitInfo, &result.image, &result.imageMemory,
                                        &result.imageView, &result.mipmappedImageView, &result.capacity);
            result.imageFormat = imageInitInfo.imageFormat;
            result.extent = {imageInitInfo.extent.width, imageInitInfo.extent.height};
            return result;
        }

        static MultiframeImageContext createMultiframeContext(CoreRef core, const ImageInitInfo &imageInitInfo) {
            const uint32_t maxFramesInFlight = core.getPhysicalDevice().getMaxFramesInFlight();
            std::vector<ImageContext> result(maxFramesInFlight);

            for (uint32_t i = 0; i < maxFramesInFlight; ++i) {
                result[i] = createContext(core, imageInitInfo);
            }

            return result;
        }

        static void destroyContext(CoreRef core, const ImageContext & imgCtx) {
            const VkDevice device = core.getLogicalDevice().getLogicalDeviceHandle();
            allocator::invoke(vkDestroyImageView, device, imgCtx.imageView, nullptr);
            if (imgCtx.mipmappedImageView != imgCtx.imageView) {
                allocator::invoke(vkDestroyImageView, device, imgCtx.mipmappedImageView, nullptr);
            }
            allocator::invoke(vkDestroyImage, device, imgCtx.image, nullptr);
            allocator::invoke(vkFreeMemory, device, imgCtx.imageMemory, nullptr);
        }

        static void destroyContext(CoreRef core, const MultiframeImageContext & imgCtx) {
            for (const auto &ctx: imgCtx) {
                destroyContext(core, ctx);
            }
        }

        static MultiframeImageContext fromSwapchain(CoreRef core, SwapchainRef swapchain) {
            const auto images = swapchain.getSwapchainImages();
            const auto imageViews = swapchain.getSwapchainImageViews();
            // The size these images were created at, not the one the window is at now: a stale window size here is what puts an attachment into a framebuffer too large for it.
            const auto extent = swapchain.getCurrentExtent();

            std::vector<ImageContext> result(images.size());

            for (uint32_t i = 0; i < images.size(); ++i) {
                result[i].image = images[i];
                result[i].imageFormat = swapchain.getImageFormat(),
                result[i].imageMemory = VK_NULL_HANDLE;
                result[i].imageView = imageViews[i];
                result[i].mipmappedImageView = imageViews[i];
                result[i].extent = extent;
                result[i].capacity = 0;
            }
            return result;
        }
    };
}
