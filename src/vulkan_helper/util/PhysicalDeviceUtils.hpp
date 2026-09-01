//
// Created by Merutilm on 2025-08-24.
// Modified by Opus 5 on 2026-08-24, 2026-08-26, 2026-08-31
//

#pragma once
#include "../core/config.hpp"
#include "../struct/QueueFamilyIndices.hpp"
#include "../struct/StringHasher.hpp"

namespace merutilm::vkh {
    struct PhysicalDeviceUtils {

        inline static const std::vector<const char *> PHYSICAL_DEVICE_EXTENSIONS = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        // Every compute pass is dispatched over a work group of this size on both axes.
        static constexpr uint32_t REQUIRED_WORK_GROUP_SIZE = 16;

        explicit PhysicalDeviceUtils() = delete;


        static bool isDeviceSuitable(const VkPhysicalDevice physicalDevice, const VkSurfaceKHR surface) {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physicalDevice, &properties);

            VkPhysicalDeviceFeatures features;
            vkGetPhysicalDeviceFeatures(physicalDevice, &features);

            const auto indices = findQueueFamilies(physicalDevice, surface);
            return properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
                   features.geometryShader &&
                   features.shaderFloat64 &&
                   checkComputeLimits(properties.limits) &&
                   indices.isComplete() &&
                   checkDeviceExtensionSupport(physicalDevice) &&
                   checkSurfaceSupport(physicalDevice, surface);
        }

        // The extension being present says nothing about what the surface will actually accept, and
        // a device chosen without asking is one whose swapchain create can still fail at startup.
        static bool checkSurfaceSupport(const VkPhysicalDevice physicalDevice, const VkSurfaceKHR surface) {
            return !populateSurfaceFormats(physicalDevice, surface).empty() &&
                   !populateSurfacePresentModes(physicalDevice, surface).empty();
        }

        static std::vector<VkSurfaceFormatKHR> populateSurfaceFormats(const VkPhysicalDevice physicalDevice,
                                                                      const VkSurfaceKHR surface) {
            uint32_t count = 0;
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, nullptr);
            std::vector<VkSurfaceFormatKHR> formats(count);
            if (count > 0) {
                vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, formats.data());
            }
            return formats;
        }

        static std::vector<VkPresentModeKHR> populateSurfacePresentModes(const VkPhysicalDevice physicalDevice,
                                                                        const VkSurfaceKHR surface) {
            uint32_t count = 0;
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, nullptr);
            std::vector<VkPresentModeKHR> presentModes(count);
            if (count > 0) {
                vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, presentModes.data());
            }
            return presentModes;
        }

        // The format the whole present chain is built for comes first; a surface offering only the
        // byte-swapped one, or only a wider color space, is served rather than refused. Component
        // order is the format's own business, so the fragment shader still writes plain RGBA.
        static VkSurfaceFormatKHR pickSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats) {
            static constexpr std::array PREFERRED = {
                VkSurfaceFormatKHR{config::SWAPCHAIN_IMAGE_FORMAT, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
                VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
            };
            for (const auto &[format, colorSpace]: PREFERRED) {
                for (const auto &available: formats) {
                    if (available.format == format && available.colorSpace == colorSpace) {
                        return available;
                    }
                }
            }
            for (const auto &available: formats) {
                if (available.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    return available;
                }
            }
            return formats.empty()
                       ? VkSurfaceFormatKHR{config::SWAPCHAIN_IMAGE_FORMAT, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}
                       : formats.front();
        }

        // FIFO is the one mode Vulkan guarantees, so it is what a driver without MAILBOX falls back to.
        static VkPresentModeKHR pickPresentMode(const std::vector<VkPresentModeKHR> &presentModes) {
            return std::ranges::find(presentModes, VK_PRESENT_MODE_MAILBOX_KHR) != presentModes.end()
                       ? VK_PRESENT_MODE_MAILBOX_KHR
                       : VK_PRESENT_MODE_FIFO_KHR;
        }

        // Opaque is what the window wants, but a compositor may only offer the pre/post-multiplied
        // bits, and one of the supported ones has to be named or the create is invalid.
        static VkCompositeAlphaFlagBitsKHR pickCompositeAlpha(const VkCompositeAlphaFlagsKHR supported) {
            static constexpr std::array PREFERRED = {
                VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
                VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            };
            for (const auto bit: PREFERRED) {
                if (supported & bit) {
                    return bit;
                }
            }
            return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        }

        // Vulkan only guarantees 128 invocations per work group, and the compute passes ask for 256.
        static bool checkComputeLimits(const VkPhysicalDeviceLimits &limits) {
            constexpr uint32_t invocations = REQUIRED_WORK_GROUP_SIZE * REQUIRED_WORK_GROUP_SIZE;
            return limits.maxComputeWorkGroupInvocations >= invocations &&
                   limits.maxComputeWorkGroupSize[0] >= REQUIRED_WORK_GROUP_SIZE &&
                   limits.maxComputeWorkGroupSize[1] >= REQUIRED_WORK_GROUP_SIZE;
        }

        static QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice physicalDevice, const VkSurfaceKHR surface) {
            QueueFamilyIndices indices;
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());
            for (uint32_t i = 0; i < queueFamilyCount; i++) {
                if (const auto &queueFamily = queueFamilies[i]; (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                    indices.graphicsAndComputeFamily = i;
                }

                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
                if (presentSupport) {
                    indices.presentFamily = i;
                }

                if (indices.isComplete()) {
                    break;
                }
            }
            return indices;
        }


        static bool checkDeviceExtensionSupport(const VkPhysicalDevice physicalDevice) {
            // Left uninitialized, the length the vector is built from is whatever was on the stack
            // when the query failed, and the names walked below are then read out of memory that
            // nothing ever wrote. The surface queries above are counted and guarded this same way.
            uint32_t extensionCount = 0;
            if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr) !=
                VK_SUCCESS) {
                return false;
            }
            std::vector<VkExtensionProperties> availableExtensions(extensionCount);
            if (extensionCount > 0 &&
                vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount,
                                                     availableExtensions.data()) != VK_SUCCESS) {
                return false;
            }
            auto required = std::unordered_set<std::string, StringHasher, std::equal_to<> >(
                PHYSICAL_DEVICE_EXTENSIONS.begin(),
                PHYSICAL_DEVICE_EXTENSIONS.end());
            for (const auto &[extensionName, specVersion]: availableExtensions) {
                required.erase(extensionName);
            }
            return required.empty();
        }
    };
}
