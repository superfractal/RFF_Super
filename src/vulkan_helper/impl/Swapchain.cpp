//
// Created by Merutilm on 2025-07-09.
// Modified by Opus 5 on 2026-08-15, 2026-08-26
// Modified by GPT-5 on 2026-08-23, 2026-09-01
//

#include "Swapchain.hpp"

#include "../core/vkh_core.hpp"
#include "../util/BufferImageUtils.hpp"
#include "../util/PhysicalDeviceUtils.hpp"

namespace merutilm::vkh {
    SwapchainImpl::SwapchainImpl(CoreRef core, SurfaceRef surface) : CoreHandler(core), surface(surface) {
        SwapchainImpl::init();
    }

    SwapchainImpl::~SwapchainImpl() {
        SwapchainImpl::destroy();
    }


    void SwapchainImpl::recreate() {
        const VkDevice device = core.getLogicalDevice().getLogicalDeviceHandle();
        const VkExtent2D newExtent = populateSwapchainExtent();
        VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;
        std::vector<VkImage> newImages;
        std::vector<VkImageView> newImageViews;
        createSwapchain(&newSwapchain, swapchain, newExtent);
        try {
            setupSwapchainImages(newSwapchain, &newImages, &newImageViews);
        } catch (...) {
            allocator::invoke(vkDestroySwapchainKHR, device, newSwapchain, nullptr);
            throw;
        }

        destroyImageViews();
        allocator::invoke(vkDestroySwapchainKHR, device, swapchain, nullptr);
        swapchain = newSwapchain;
        swapchainImages = std::move(newImages);
        swapchainImageViews = std::move(newImageViews);
        currentExtent = newExtent;
    }


    VkExtent2D SwapchainImpl::populateSwapchainExtent() const {
        const HWND window = surface.getTargetWindow().getWindowHandle();
        const auto capabilities = core.getPhysicalDevice().populateSurfaceCapabilities(surface.getSurfaceHandle());

        // A surface that names its own size must be taken at its word; the special width means only
        // that it has none of its own, and the window's is what fills it in.
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        }

        RECT rect;
        GetClientRect(window, &rect);
        const VkExtent2D extent = {
            .width = std::clamp(static_cast<uint32_t>(rect.right - rect.left), capabilities.minImageExtent.width,
                                capabilities.maxImageExtent.width),
            .height = std::clamp(static_cast<uint32_t>(rect.bottom - rect.top), capabilities.minImageExtent.height,
                                 capabilities.maxImageExtent.height),

        };
        return extent;
    }


    void SwapchainImpl::init() {
        pickSurfaceProperties();
        const VkExtent2D newExtent = populateSwapchainExtent();
        VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;
        std::vector<VkImage> newImages;
        std::vector<VkImageView> newImageViews;
        createSwapchain(&newSwapchain, nullptr, newExtent);
        try {
            setupSwapchainImages(newSwapchain, &newImages, &newImageViews);
        } catch (...) {
            allocator::invoke(vkDestroySwapchainKHR, core.getLogicalDevice().getLogicalDeviceHandle(),
                              newSwapchain, nullptr);
            throw;
        }
        swapchain = newSwapchain;
        swapchainImages = std::move(newImages);
        swapchainImageViews = std::move(newImageViews);
        currentExtent = newExtent;
    }

    void SwapchainImpl::pickSurfaceProperties() {
        const VkPhysicalDevice physicalDevice = core.getPhysicalDevice().getPhysicalDeviceHandle();
        const VkSurfaceKHR surfaceHandle = surface.getSurfaceHandle();
        surfaceFormat = PhysicalDeviceUtils::pickSurfaceFormat(
            PhysicalDeviceUtils::populateSurfaceFormats(physicalDevice, surfaceHandle));
        presentMode = PhysicalDeviceUtils::pickPresentMode(
            PhysicalDeviceUtils::populateSurfacePresentModes(physicalDevice, surfaceHandle));
        compositeAlpha = PhysicalDeviceUtils::pickCompositeAlpha(
            core.getPhysicalDevice().populateSurfaceCapabilities(surfaceHandle).supportedCompositeAlpha);
    }

    void SwapchainImpl::createSwapchain(VkSwapchainKHR *target, const VkSwapchainKHR old,
                                        const VkExtent2D extent) const {
        const uint32_t maxFramesInFlight = core.getPhysicalDevice().getMaxFramesInFlight();
        const auto &[graphicsFamily, presentFamily] = core.getPhysicalDevice().getQueueFamilyIndices();
        std::array queueFamilyIndices = {graphicsFamily.value(), presentFamily.value()};

        const VkSurfaceCapabilitiesKHR capabilities = core.getPhysicalDevice().populateSurfaceCapabilities(surface.getSurfaceHandle());

        // The frames in flight are the count asked for, but the surface's own bounds decide it.
        uint32_t minImageCount = std::max(maxFramesInFlight, capabilities.minImageCount);
        if (capabilities.maxImageCount > 0 && minImageCount > capabilities.maxImageCount) {
            minImageCount = capabilities.maxImageCount;
        }

        if (const VkSwapchainCreateInfoKHR createInfo = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .surface = surface.getSurfaceHandle(),
            .minImageCount = minImageCount,
            .imageFormat = surfaceFormat.format,
            .imageColorSpace = surfaceFormat.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = graphicsFamily == presentFamily
                                    ? VK_SHARING_MODE_EXCLUSIVE
                                    : VK_SHARING_MODE_CONCURRENT,
            .queueFamilyIndexCount = graphicsFamily == presentFamily
                                         ? 0
                                         : static_cast<uint32_t>(queueFamilyIndices.size()),
            .pQueueFamilyIndices = graphicsFamily == presentFamily ? nullptr : queueFamilyIndices.data(),
            .preTransform = capabilities.currentTransform,
            .compositeAlpha = compositeAlpha,
            .presentMode = presentMode,
            .clipped = VK_TRUE,
            .oldSwapchain = old
        }; allocator::invoke(vkCreateSwapchainKHR, core.getLogicalDevice().getLogicalDeviceHandle(), &createInfo, nullptr, target) !=
           VK_SUCCESS) {
            throw exception_init("Failed to create swapchain!");
        }
    }

    void SwapchainImpl::setupSwapchainImages(const VkSwapchainKHR target, std::vector<VkImage> *images,
                                              std::vector<VkImageView> *imageViews) const {
        const VkDevice device = core.getLogicalDevice().getLogicalDeviceHandle();
        VkResult result = VK_INCOMPLETE;
        uint32_t imageCount = 0;
        while (result == VK_INCOMPLETE) {
            if (allocator::invoke(vkGetSwapchainImagesKHR, device, target, &imageCount, nullptr) != VK_SUCCESS ||
                imageCount == 0) {
                throw exception_init("Failed to query swapchain images!");
            }
            images->resize(imageCount);
            result = allocator::invoke(vkGetSwapchainImagesKHR, device, target, &imageCount,
                                       images->data());
        }
        if (result != VK_SUCCESS) {
            throw exception_init("Failed to get swapchain images!");
        }
        images->resize(imageCount);
        imageViews->assign(imageCount, VK_NULL_HANDLE);
        try {
            for (uint32_t i = 0; i < imageCount; ++i) {
                BufferImageUtils::createImageView(device, (*images)[i], VK_IMAGE_VIEW_TYPE_2D,
                                                  surfaceFormat.format, &(*imageViews)[i]);
            }
        } catch (...) {
            for (const VkImageView imageView: *imageViews) {
                if (imageView != VK_NULL_HANDLE) {
                    allocator::invoke(vkDestroyImageView, device, imageView, nullptr);
                }
            }
            imageViews->clear();
            images->clear();
            throw;
        }
    }


    void SwapchainImpl::destroy() {
        destroyImageViews();
        if (swapchain != VK_NULL_HANDLE) {
            allocator::invoke(vkDestroySwapchainKHR, core.getLogicalDevice().getLogicalDeviceHandle(), swapchain,
                              nullptr);
            swapchain = VK_NULL_HANDLE;
        }
        swapchainImages.clear();
    }

    void SwapchainImpl::destroyImageViews() {
        for (const VkImageView imageView: swapchainImageViews) {
            if (imageView != VK_NULL_HANDLE) {
                allocator::invoke(vkDestroyImageView, core.getLogicalDevice().getLogicalDeviceHandle(), imageView,
                                  nullptr);
            }
        }
        swapchainImageViews.clear();
    }
}
