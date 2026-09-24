//
// Created by Merutilm on 2025-08-29.
// Modified by GPT-6 on 2026-09-23
//

#include "Fence.hpp"
#include "../core/exception.hpp"


namespace merutilm::vkh {
    FenceImpl::FenceImpl(CoreRef core) : CoreHandler(core) {
        FenceImpl::init();
    }

    FenceImpl::~FenceImpl() {
        FenceImpl::destroy();
    }

    void FenceImpl::init() {
        if (fence != VK_NULL_HANDLE) {
            throw exception_invalid_state("Fence is already initialized");
        }

        const VkDevice device = core.getLogicalDevice().getLogicalDeviceHandle();

        constexpr VkFenceCreateInfo fenceInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        VkFence createdFence = VK_NULL_HANDLE;
        if (allocator::invoke(vkCreateFence, device, &fenceInfo, nullptr, &createdFence) != VK_SUCCESS) {
            throw exception_init("Failed to create fence!");
        }
        fence = createdFence;
    }

    void FenceImpl::destroy() {
        if (fence == VK_NULL_HANDLE) {
            return;
        }

        const VkDevice device = core.getLogicalDevice().getLogicalDeviceHandle();
        allocator::invoke(vkDestroyFence, device, fence, nullptr);
        fence = VK_NULL_HANDLE;
    }
}
