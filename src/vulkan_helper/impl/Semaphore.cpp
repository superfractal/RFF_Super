//
// Created by Merutilm on 2025-09-01.
// Modified by GPT-6 on 2026-09-23
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
        if (imageAvailable != VK_NULL_HANDLE || renderFinished != VK_NULL_HANDLE) {
            throw exception_invalid_state("Semaphores are already initialized");
        }
        const VkDevice device = core.getLogicalDevice().getLogicalDeviceHandle();
        constexpr VkSemaphoreCreateInfo semaphoreInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0
        };

        VkSemaphore createdImageAvailable = VK_NULL_HANDLE;
        VkSemaphore createdRenderFinished = VK_NULL_HANDLE;
        if (allocator::invoke(vkCreateSemaphore, device, &semaphoreInfo,
                              nullptr, &createdImageAvailable) != VK_SUCCESS) {
            throw exception_init("Failed to create sync objects!");
        }
        try {
            if (allocator::invoke(vkCreateSemaphore, device, &semaphoreInfo,
                                  nullptr, &createdRenderFinished) != VK_SUCCESS) {
                throw exception_init("Failed to create sync objects!");
            }
        } catch (...) {
            allocator::invoke(vkDestroySemaphore, device, createdImageAvailable, nullptr);
            throw;
        }
        imageAvailable = createdImageAvailable;
        renderFinished = createdRenderFinished;
    }

    void SemaphoreImpl::destroy() {
        if (imageAvailable == VK_NULL_HANDLE && renderFinished == VK_NULL_HANDLE) {
            return;
        }
        const VkDevice device = core.getLogicalDevice().getLogicalDeviceHandle();
        if (imageAvailable != VK_NULL_HANDLE) {
            allocator::invoke(vkDestroySemaphore, device, imageAvailable, nullptr);
            imageAvailable = VK_NULL_HANDLE;
        }
        if (renderFinished != VK_NULL_HANDLE) {
            allocator::invoke(vkDestroySemaphore, device, renderFinished, nullptr);
            renderFinished = VK_NULL_HANDLE;
        }
    }
}
