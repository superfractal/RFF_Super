//
// Created by Merutilm on 2025-07-09.
// Modified by Opus 5 on 2026-08-23
// Modified by GPT-5 on 2026-08-23
// Modified by GPT-6 on 2026-09-23
//

#include "LogicalDevice.hpp"

#include <array>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <new>
#include <queue>
#include <string>
#include <vector>
#include <windows.h>

#include "../core/exception.hpp"
#include "../core/config.hpp"
#include "../util/Debugger.hpp"
#include "../util/PhysicalDeviceUtils.hpp"

namespace merutilm::vkh {
    namespace {
        constexpr std::streamsize MAX_PIPELINE_CACHE_SIZE = 256 * 1024 * 1024;
        std::atomic<uint64_t> cacheTemporaryCounter{0};

        std::filesystem::path pipelineCachePath() {
            std::array<wchar_t, MAX_PATH> buffer = {};
            GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
            return std::filesystem::path(buffer.data()).parent_path() / L"pipeline-cache.bin";
        }

        std::vector<char> readPipelineCache() {
            std::ifstream in(pipelineCachePath(), std::ios::binary | std::ios::ate);
            if (!in) {
                return {};
            }
            const std::streamsize size = in.tellg();
            // The header alone is 32 bytes, so anything shorter cannot even name the device it came from.
            if (size <= 32 || size > MAX_PIPELINE_CACHE_SIZE) {
                return {};
            }
            std::vector<char> data;
            try {
                data.resize(static_cast<size_t>(size));
            } catch (const std::bad_alloc &) {
                return {};
            }
            in.seekg(0);
            in.read(data.data(), size);
            if (!in) {
                return {};
            }
            return data;
        }
    }

    LogicalDeviceImpl::LogicalDeviceImpl(PhysicalDeviceLoaderRef physicalDevice)
        : physicalDevice(physicalDevice) {
        LogicalDeviceImpl::init();
    }

    LogicalDeviceImpl::~LogicalDeviceImpl() {
        LogicalDeviceImpl::destroy();
    }

    void LogicalDeviceImpl::init() {
        if (logicalDevice != VK_NULL_HANDLE) {
            throw exception_invalid_state("Logical device is already initialized");
        }
        float queuePriority = 1;
        const auto &[graphicsFamily, presentFamily] = physicalDevice.getQueueFamilyIndices();
        std::array<VkDeviceQueueCreateInfo, 2> queueCreateInfos = {};
        queueCreateInfos[0] = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = graphicsFamily.value(),
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        };
        uint32_t queueCreateInfoCount = 1;
        if (presentFamily != graphicsFamily) {
            queueCreateInfos[1] = queueCreateInfos[0];
            queueCreateInfos[1].queueFamilyIndex = presentFamily.value();
            queueCreateInfoCount = 2;
        }

        const VkDeviceCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueCreateInfoCount = queueCreateInfoCount,
            .pQueueCreateInfos = queueCreateInfos.data(),
            .enabledLayerCount = config::ENABLE_VALIDATION ? 1 : 0,
            .ppEnabledLayerNames = config::ENABLE_VALIDATION ? &Debugger::VALIDATION_LAYER : nullptr,
            .enabledExtensionCount = static_cast<uint32_t>(PhysicalDeviceUtils::PHYSICAL_DEVICE_EXTENSIONS.size()),
            .ppEnabledExtensionNames = PhysicalDeviceUtils::PHYSICAL_DEVICE_EXTENSIONS.data(),
            .pEnabledFeatures = &physicalDevice.getPhysicalDeviceFeatures()
        };
        VkDevice createdDevice = VK_NULL_HANDLE;
        if (allocator::invoke(vkCreateDevice, physicalDevice.getPhysicalDeviceHandle(),
                              &createInfo, nullptr, &createdDevice) != VK_SUCCESS) {
            throw exception_init("failed to create logical device!");
        }
        logicalDevice = createdDevice;
        try {
            vkGetDeviceQueue(logicalDevice, physicalDevice.getQueueFamilyIndices().graphicsAndComputeFamily.value(),
                             0, &graphicsQueue);
            vkGetDeviceQueue(logicalDevice, physicalDevice.getQueueFamilyIndices().presentFamily.value(),
                             0, &presentQueue);

            // Data built by another device or driver is rejected by the implementation itself, which then
            // starts the cache empty - so a stale file costs the compile it would have cost anyway.
            const std::vector<char> cached = readPipelineCache();
            const VkPipelineCacheCreateInfo cacheInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .initialDataSize = cached.size(),
                .pInitialData = cached.empty() ? nullptr : cached.data(),
            };
            VkPipelineCache createdCache = VK_NULL_HANDLE;
            if (allocator::invoke(vkCreatePipelineCache, logicalDevice, &cacheInfo,
                                  nullptr, &createdCache) != VK_SUCCESS) {
                // Every pipeline is then compiled from scratch, which is slower to start and nothing worse.
                pipelineCache = VK_NULL_HANDLE;
            } else {
                pipelineCache = createdCache;
            }
        } catch (...) {
            destroy();
            throw;
        }
    }

    void LogicalDeviceImpl::savePipelineCache() const {
        if (pipelineCache == nullptr) {
            return;
        }
        size_t size = 0;
        if (allocator::invoke(vkGetPipelineCacheData, logicalDevice, pipelineCache, &size, nullptr) != VK_SUCCESS ||
            size == 0 || size > static_cast<size_t>(MAX_PIPELINE_CACHE_SIZE)) {
            return;
        }
        std::vector<char> data(size);
        if (allocator::invoke(vkGetPipelineCacheData, logicalDevice, pipelineCache, &size, data.data()) !=
            VK_SUCCESS) {
            return;
        }
        const auto target = pipelineCachePath();
        std::filesystem::path temporary = target;
        temporary += L"." + std::to_wstring(GetCurrentProcessId()) + L"-" +
                     std::to_wstring(cacheTemporaryCounter.fetch_add(1)) + L".tmp";
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        if (out) {
            out.write(data.data(), static_cast<std::streamsize>(size));
            out.close();
            if (out && MoveFileExW(temporary.c_str(), target.c_str(),
                                   MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
                return;
            }
        }
        std::error_code error;
        std::filesystem::remove(temporary, error);
    }


    void LogicalDeviceImpl::destroy() {
        if (logicalDevice == VK_NULL_HANDLE) {
            return;
        }
        if (pipelineCache != VK_NULL_HANDLE) {
            try {
                savePipelineCache();
            } catch (...) {
            }
            allocator::invoke(vkDestroyPipelineCache, logicalDevice, pipelineCache, nullptr);
            pipelineCache = VK_NULL_HANDLE;
        }
        allocator::invoke(vkDestroyDevice, logicalDevice, nullptr);
        logicalDevice = VK_NULL_HANDLE;
        graphicsQueue = VK_NULL_HANDLE;
        presentQueue = VK_NULL_HANDLE;
    }
}
