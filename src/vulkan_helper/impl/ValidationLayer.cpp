//
// Created by Merutilm on 2025-07-08.
// Modified by Opus 5 on 2026-08-31
// Modified by GPT-6 on 2026-09-23
//

#include "ValidationLayer.hpp"
#include "../core/vkh_core.hpp"
#include "../util/Debugger.hpp"

namespace merutilm::vkh {
    ValidationLayerImpl::ValidationLayerImpl(const VkInstance instance) : instance(instance) {
        ValidationLayerImpl::init();
    }

    ValidationLayerImpl::~ValidationLayerImpl() {
        ValidationLayerImpl::destroy();
    }

    void ValidationLayerImpl::init() {
        checkValidationLayerSupport();
        setupDebugMessenger();
    }
    void ValidationLayerImpl::checkValidationLayerSupport() {
        uint32_t layerCount = 0;
        if (vkEnumerateInstanceLayerProperties(&layerCount, nullptr) != VK_SUCCESS) {
            throw exception_init("No validation layers available");
        }
        std::vector<VkLayerProperties> availableLayers(layerCount);
        if (layerCount > 0 && vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()) != VK_SUCCESS) {
            throw exception_init("No validation layers available");
        }

        const bool hasValidationLayer = std::ranges::any_of(
            availableLayers, [](const VkLayerProperties &layerProperties) {
                return strcmp(Debugger::VALIDATION_LAYER, layerProperties.layerName) == 0;
            });
        if (!hasValidationLayer) {
            throw exception_init("No validation layers available");
        }
    }

    void ValidationLayerImpl::setupDebugMessenger() {
        const VkDebugUtilsMessengerCreateInfoEXT createInfo = Debugger::populateDebugMessengerCreateInfo();
        if (allocator::invoke(createDebugUtilsMessengerEXT, instance, &createInfo, nullptr, &debugMessenger) !=
            VK_SUCCESS) {
            throw exception_init("Failed to create debug messenger");
        }
    }


    VkResult ValidationLayerImpl::createDebugUtilsMessengerEXT(const VkInstance instance,
                                                           const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                                           const VkAllocationCallbacks *pAllocator,
                                                           VkDebugUtilsMessengerEXT *pDebugMessenger) {
        const auto createMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT"));
        if (createMessenger == nullptr) {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
        return createMessenger(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }


    void ValidationLayerImpl::destroyDebugUtilsMessengerEXT(const VkInstance instance,
                                                        const VkDebugUtilsMessengerEXT debugMessenger,
                                                        const VkAllocationCallbacks *pAllocator) {
        const auto destroyMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (destroyMessenger == nullptr) {
            return;
        }
        destroyMessenger(instance, debugMessenger, pAllocator);
    }

    void ValidationLayerImpl::destroy() {
        allocator::invoke(destroyDebugUtilsMessengerEXT, instance, debugMessenger, nullptr);
    }
}
