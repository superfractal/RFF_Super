//
// Created by Merutilm on 2025-09-01.
// Modified by GPT-6 on 2026-09-23, 2026-09-25
//

#include "Semaphore.hpp"

#include "../core/allocator.hpp"
#include "../core/exception.hpp"

namespace merutilm::vkh {
    SemaphoreImpl::SemaphoreImpl(CoreRef core) : CoreHandler(core) {
        SemaphoreImpl::init();
    }

    SemaphoreImpl::~SemaphoreImpl() {
        SemaphoreImpl::destroy();
    }

    void SemaphoreImpl::init() {
        if (handle != VK_NULL_HANDLE) {
            throw exception_invalid_state("Semaphore is already initialized");
        }
        const VkDevice device = core.getLogicalDevice().getLogicalDeviceHandle();
        constexpr VkSemaphoreCreateInfo semaphoreInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0
        };
        if (allocator::invoke(vkCreateSemaphore, device, &semaphoreInfo,
                              nullptr, &handle) != VK_SUCCESS) {
            throw exception_init("Failed to create semaphore!");
        }
    }

    void SemaphoreImpl::destroy() {
        if (handle != VK_NULL_HANDLE) {
            allocator::invoke(vkDestroySemaphore, core.getLogicalDevice().getLogicalDeviceHandle(), handle, nullptr);
            handle = VK_NULL_HANDLE;
        }
    }
}
