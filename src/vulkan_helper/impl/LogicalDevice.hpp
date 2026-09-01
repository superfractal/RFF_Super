//
// Created by Merutilm on 2025-07-09.
// Modified by Opus 5 on 2026-08-09, 2026-08-23
// Modified by GPT-5 on 2026-08-23.
//

#pragma once
#include <mutex>

#include <vulkan/vk_enum_string_helper.h>

#include "Instance.hpp"
#include "PhysicalDeviceLoader.hpp"
#include "../core/allocator.hpp"
#include "../handle/Handler.hpp"

namespace merutilm::vkh {
    class LogicalDeviceImpl final : public Handler {
        InstanceRef instance;
        PhysicalDeviceLoaderRef physicalDevice;
        VkDevice logicalDevice = nullptr;
        VkQueue graphicsQueue = nullptr;
        // Compiled pipelines, kept from run to run in a file beside the executable. A cold compile of
        // the video chain's compute shader costs minutes on some drivers; this pays for it once.
        VkPipelineCache pipelineCache = nullptr;
        VkQueue presentQueue = nullptr;
        // A queue may be used by one thread at a time. The keyframe preview draws on a thread of its
        // own while the main window keeps drawing on its context, and both submit through here.
        std::mutex queueMutex;

    public:
        explicit LogicalDeviceImpl(InstanceRef instance, PhysicalDeviceLoaderRef physicalDevice);

        ~LogicalDeviceImpl() override;

        LogicalDeviceImpl(const LogicalDeviceImpl &) = delete;

        LogicalDeviceImpl &operator=(const LogicalDeviceImpl &) = delete;

        LogicalDeviceImpl(LogicalDeviceImpl &&) = delete;

        LogicalDeviceImpl &operator=(LogicalDeviceImpl &&) = delete;

        [[nodiscard]] VkDevice getLogicalDeviceHandle() const { return logicalDevice; }

        [[nodiscard]] VkQueue getGraphicsQueue() const { return graphicsQueue; }

        [[nodiscard]] VkPipelineCache getPipelineCacheHandle() const { return pipelineCache; }

        // Writes what has been compiled so far. Called on shutdown, and wherever a long compile has
        // just been paid for, so an end that never reaches shutdown does not throw the work away.
        void savePipelineCache() const;

        [[nodiscard]] VkQueue getPresentQueue() const { return presentQueue; }

        // The result code is the only clue these two carry: every other Vulkan call in the
        // codebase discards its VkResult, so a failure that started elsewhere (a rejected
        // allocation, a lost device) first becomes visible here. Name it in the message.
        void queueSubmit(const uint32_t submitCount, const VkSubmitInfo *pSubmits, const VkFence fence) {
            std::scoped_lock lock(queueMutex);
            if (const VkResult result = allocator::invoke(vkQueueSubmit, graphicsQueue, submitCount, pSubmits, fence);
                result != VK_SUCCESS) {
                throw exception_invalid_state(std::string("Failed to submit queue! ") + string_VkResult(result));
            }
        }

        VkResult queuePresent(const VkPresentInfoKHR *presentInfo) {
            std::scoped_lock lock(queueMutex);
            if (const VkResult result = allocator::invoke(vkQueuePresentKHR, presentQueue, presentInfo);
                result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR && result != VK_ERROR_OUT_OF_DATE_KHR) {
                throw exception_invalid_state(std::string("Failed to present queue! ") + string_VkResult(result));
            } else {
                return result;
            }
        }

        void waitDeviceIdle() {
            std::scoped_lock lock(queueMutex);
            allocator::invoke(vkDeviceWaitIdle, logicalDevice);
        }

    private:
        void init() override;

        void destroy() override;
    };

    using LogicalDevice = std::unique_ptr<LogicalDeviceImpl>;
    using LogicalDevicePtr = LogicalDeviceImpl *;
    using LogicalDeviceRef = LogicalDeviceImpl &;
}
