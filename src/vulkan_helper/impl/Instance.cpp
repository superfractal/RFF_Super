//
// Created by Merutilm on 2025-07-08.
// Modified by Opus 5 on 2026-08-09
//

#include "Instance.hpp"

#include "../core/factory.hpp"
#include "../core/exception.hpp"
#include "../core/config.hpp"
#include "../util/Debugger.hpp"

namespace merutilm::vkh {

    InstanceImpl::InstanceImpl() {
        InstanceImpl::init();
    }

    InstanceImpl::~InstanceImpl() {
        InstanceImpl::destroy();
    }

    void InstanceImpl::init() {
        createInstance();
        if constexpr(config::ENABLE_VALIDATION) {
            validationLayer = factory::create<ValidationLayer>(instance);
        }else {
            validationLayer = nullptr;
        }

    }

    void InstanceImpl::createInstance() {
        VkApplicationInfo applicationInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = "Application",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "1.0.0",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_0,
        };
        if constexpr (config::ENABLE_VALIDATION) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = Debugger::populateDebugMessengerCreateInfo();
        // VUID-VkInstanceCreateInfo-pNext-04926: the messenger may only be chained when
        // VK_EXT_debug_utils is enabled, which only happens under ENABLE_VALIDATION. Chaining it
        // unconditionally made every release build violate the spec and silently disabled the
        // application's own debug callback.
        const VkInstanceCreateInfo instanceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = config::ENABLE_VALIDATION ? &debugMessengerCreateInfo : nullptr,
            .flags = 0,
            .pApplicationInfo = &applicationInfo,
            .enabledLayerCount = config::ENABLE_VALIDATION ? 1u : 0,
            .ppEnabledLayerNames =  config::ENABLE_VALIDATION ? &Debugger::VALIDATION_LAYER : nullptr,
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data(),
        };

        if (allocator::invoke(vkCreateInstance, &instanceCreateInfo, nullptr, &instance)) {
            throw exception_init("Failed to create instance!");
        }

    }

    void InstanceImpl::destroy() {
        validationLayer = nullptr;
        allocator::invoke(vkDestroyInstance, instance, nullptr);
    }
}
