//
// Created by Merutilm on 2025-08-24.
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include "../core/vkh_core.hpp"

namespace merutilm::vkh {
    struct Debugger {
        explicit Debugger() = delete;

        static constexpr auto VALIDATION_LAYER = "VK_LAYER_KHRONOS_validation";

        static VkDebugUtilsMessengerCreateInfoEXT populateDebugMessengerCreateInfo() {
            return {
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .pNext = nullptr,
                .flags = 0,
                .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,
                .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
                .pfnUserCallback = debugCallback,
                .pUserData = nullptr
            };
        }

        static VkBool32 debugCallback(const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                      const VkDebugUtilsMessageTypeFlagsEXT messageType,
                                      const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                      [[maybe_unused]] void *pUserData) {
            if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
                logger::log_err_silent("{} {} : {}",
                                       severityLabel(messageSeverity),
                                       messageTypeLabel(messageType),
                                       pCallbackData->pMessage);
            }

            return VK_FALSE;
        }

    private:
        static const char *severityLabel(const VkDebugUtilsMessageSeverityFlagBitsEXT severity) {
            switch (severity) {
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                    return "[Warning]";
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                    return "[Error]";
                default:
                    return "[Unknown]";
            }
        }

        static const char *messageTypeLabel(const VkDebugUtilsMessageTypeFlagsEXT type) {
            switch (type) {
                case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
                    return "[General]";
                case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
                    return "[Validation]";
                case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
                    return "[Performance]";
                case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
                    return "[DeviceAddressBinding]";
                default:
                    return "[Unknown]";
            }
        }
    };
}
