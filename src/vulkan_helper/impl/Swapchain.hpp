//
// Created by Merutilm on 2025-07-09.
// Modified by Opus 5 on 2026-08-15, 2026-08-26
// Modified by GPT-5 on 2026-09-01
//

#pragma once
#include "../core/vkh_base.hpp"
#include "../handle/CoreHandler.hpp"

namespace merutilm::vkh {
    class SwapchainImpl final : public CoreHandler {

        SurfaceRef surface;
        VkSwapchainKHR swapchain = nullptr;
        std::vector<VkImage> swapchainImages = {};
        std::vector<VkImageView> swapchainImageViews = {};
        // The size the present images were really created at, which populateSwapchainExtent() stops reporting once the window moves on without a recreate.
        VkExtent2D currentExtent = {};
        // What the surface actually offers, picked once at init: none of the three is guaranteed by
        // Vulkan, and naming an unsupported one fails the create on an otherwise perfectly good GPU.
        VkSurfaceFormatKHR surfaceFormat = {};
        VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
        VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    public:
        explicit SwapchainImpl(CoreRef core, SurfaceRef surface);

        ~SwapchainImpl() override;

        SwapchainImpl(const SwapchainImpl &) = delete;

        SwapchainImpl &operator=(const SwapchainImpl &) = delete;

        SwapchainImpl(SwapchainImpl &&) = delete;

        SwapchainImpl &operator=(SwapchainImpl &&) = delete;

        void recreate();

        [[nodiscard]] VkExtent2D populateSwapchainExtent() const;

        [[nodiscard]] VkExtent2D getCurrentExtent() const { return currentExtent; }

        // The format the present images were really created with, which the render pass built over
        // them has to name too.
        [[nodiscard]] VkFormat getImageFormat() const { return surfaceFormat.format; }

        [[nodiscard]] VkSwapchainKHR getSwapchainHandle() const { return swapchain; }

        [[nodiscard]] std::span<const VkImage> getSwapchainImages() const { return swapchainImages; }

        [[nodiscard]] std::span<const VkImageView> getSwapchainImageViews() const { return swapchainImageViews; }

    private:
        void init() override;

        void pickSurfaceProperties();

        void createSwapchain(VkSwapchainKHR *target, VkSwapchainKHR old, VkExtent2D extent) const;

        void setupSwapchainImages(VkSwapchainKHR target, std::vector<VkImage> *images,
                                  std::vector<VkImageView> *imageViews) const;

        void destroy() override;

        void destroyImageViews();
    };

    using Swapchain = std::unique_ptr<SwapchainImpl>;
    using SwapchainPtr = SwapchainImpl *;
    using SwapchainRef = SwapchainImpl &;
}
