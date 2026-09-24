//
// Created by Merutilm on 2025-07-09.
// Modified by GPT-6 on 2026-09-23
//

#include "CommandPool.hpp"

#include "../core/exception.hpp"
#include "../core/vkh_core.hpp"

namespace merutilm::vkh {
    CommandPoolImpl::CommandPoolImpl(CoreRef core)
        : CoreHandler(core) {
        CommandPoolImpl::init();
    }

    CommandPoolImpl::~CommandPoolImpl() {
        CommandPoolImpl::destroy();
    }
    void CommandPoolImpl::init() {
        if (commandPool != VK_NULL_HANDLE) {
            throw exception_invalid_state("Command pool is already initialized");
        }
        const VkDevice device = core.getLogicalDevice().getLogicalDeviceHandle();
        const uint32_t queueFamilyIndex =
            core.getPhysicalDevice().getQueueFamilyIndices().graphicsAndComputeFamily.value();

        const VkCommandPoolCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = queueFamilyIndex,
        };

        VkCommandPool createdCommandPool = VK_NULL_HANDLE;
        if (allocator::invoke(vkCreateCommandPool, device, &createInfo, nullptr, &createdCommandPool) != VK_SUCCESS) {
            throw exception_init("Failed to create command pool!");
        }
        commandPool = createdCommandPool;
    }

    void CommandPoolImpl::destroy() {
        if (commandPool == VK_NULL_HANDLE) {
            return;
        }
        const VkDevice device = core.getLogicalDevice().getLogicalDeviceHandle();
        allocator::invoke(vkDestroyCommandPool, device, commandPool, nullptr);
        commandPool = VK_NULL_HANDLE;
    }
}
