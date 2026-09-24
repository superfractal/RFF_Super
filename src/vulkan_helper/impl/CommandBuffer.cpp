//
// Created by Merutilm on 2025-07-09.
// Modified by GPT-6 on 2026-09-23
//

#include "CommandBuffer.hpp"

#include <utility>

#include "../core/allocator.hpp"
#include "../core/exception.hpp"
#include "../core/vkh_core.hpp"

namespace merutilm::vkh {
    CommandBufferImpl::CommandBufferImpl(CoreRef core, CommandPoolRef commandPool)
        : CoreHandler(core), commandPool(commandPool) {
        CommandBufferImpl::init();
    }

    CommandBufferImpl::~CommandBufferImpl() {
        CommandBufferImpl::destroy();
    }


    void CommandBufferImpl::init() {
        if (!commandBuffers.empty()) {
            throw exception_invalid_state("Command buffers are already initialized");
        }

        const VkDevice device = core.getLogicalDevice().getLogicalDeviceHandle();
        const uint32_t maxFramesInFlight = core.getPhysicalDevice().getMaxFramesInFlight();
        const VkCommandPool pool = commandPool.getCommandPoolHandle();
        const VkCommandBufferAllocateInfo allocateInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        std::vector<VkCommandBuffer> allocatedBuffers(maxFramesInFlight, VK_NULL_HANDLE);
        try {
            for (uint32_t frameIndex = 0; frameIndex < maxFramesInFlight; ++frameIndex) {
                VkCommandBuffer allocated = VK_NULL_HANDLE;
                if (allocator::invoke(vkAllocateCommandBuffers, device, &allocateInfo, &allocated) != VK_SUCCESS) {
                    throw exception_init("Failed to allocate command buffers!");
                }
                allocatedBuffers[frameIndex] = allocated;
            }
        } catch (...) {
            for (VkCommandBuffer &buffer : allocatedBuffers) {
                if (buffer != VK_NULL_HANDLE) {
                    allocator::invoke(vkFreeCommandBuffers, device, pool, 1, &buffer);
                }
            }
            throw;
        }

        commandBuffers = std::move(allocatedBuffers);
    }

    void CommandBufferImpl::destroy() {
        if (commandBuffers.empty()) {
            return;
        }

        const VkDevice device = core.getLogicalDevice().getLogicalDeviceHandle();
        const VkCommandPool pool = commandPool.getCommandPoolHandle();
        for (VkCommandBuffer &buffer : commandBuffers) {
            if (buffer != VK_NULL_HANDLE) {
                allocator::invoke(vkFreeCommandBuffers, device, pool, 1, &buffer);
                buffer = VK_NULL_HANDLE;
            }
        }
        commandBuffers.clear();
    }



}
