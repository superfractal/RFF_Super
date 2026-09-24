//
// Created by Merutilm on 2025-07-09.
// Modified by GPT-6 on 2026-09-23
//

#include "Surface.hpp"

#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_win32.h>

#include "../core/vkh_core.hpp"

namespace merutilm::vkh {
    SurfaceImpl::SurfaceImpl(InstanceRef instance, GraphicsContextWindowRef window)
        : instance(instance), window(window) {
        SurfaceImpl::init();
    }

    SurfaceImpl::~SurfaceImpl() {
        SurfaceImpl::destroy();
    }

    void SurfaceImpl::init() {
        if (surface != VK_NULL_HANDLE) {
            throw exception_invalid_state("Window surface is already initialized");
        }
        const VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .hinstance = GetModuleHandle(nullptr),
            .hwnd = window.getWindowHandle()
        };

        VkSurfaceKHR createdSurface = VK_NULL_HANDLE;
        if (allocator::invoke(vkCreateWin32SurfaceKHR, instance.getInstanceHandle(),
                              &surfaceCreateInfo, nullptr, &createdSurface) != VK_SUCCESS) {
            throw exception_init("failed to create window surface!");
        }
        surface = createdSurface;
    }

    void SurfaceImpl::destroy() {
        if (surface == VK_NULL_HANDLE) {
            return;
        }
        allocator::invoke(vkDestroySurfaceKHR, instance.getInstanceHandle(), surface, nullptr);
        surface = VK_NULL_HANDLE;
    }
}
