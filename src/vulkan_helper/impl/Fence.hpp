//
// Created by Merutilm on 2025-08-29.
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include "../core/vkh_core.hpp"
#include "Core.hpp"
#include "../handle/CoreHandler.hpp"

namespace merutilm::vkh {
    class FenceImpl final : public CoreHandler {

        VkFence fence = VK_NULL_HANDLE;
        bool unsubmitted = false;
        bool acquiredImageWithoutSubmission = false;

    public:
        explicit FenceImpl(CoreRef core);

        ~FenceImpl() override;

        FenceImpl(const FenceImpl &) = delete;

        FenceImpl &operator=(const FenceImpl &) = delete;

        FenceImpl(FenceImpl &&) = delete;

        FenceImpl &operator=(FenceImpl &&) = delete;

        [[nodiscard]] VkFence getFenceHandle() const { return fence; }

        void wait() {
            if (acquiredImageWithoutSubmission) {
                throw exception_invalid_state("Cannot reuse an acquired swapchain image after submission failed.");
            }
            if (unsubmitted) {
                return;
            }
            if (const VkResult result = allocator::invoke(vkWaitForFences,
                    core.getLogicalDevice().getLogicalDeviceHandle(), 1, &fence, VK_TRUE, UINT64_MAX);
                result != VK_SUCCESS) {
                throw exception_invalid_state(std::string("Failed to wait for fence! ") + string_VkResult(result));
            }
        }

        void reset() {
            if (const VkResult result = allocator::invoke(vkResetFences,
                    core.getLogicalDevice().getLogicalDeviceHandle(), 1, &fence); result != VK_SUCCESS) {
                throw exception_invalid_state(std::string("Failed to reset fence! ") + string_VkResult(result));
            }
        }

        void markUnsubmitted() noexcept { unsubmitted = true; }

        void markAcquiredImageWithoutSubmission() noexcept { acquiredImageWithoutSubmission = true; }

        void markSubmitted() noexcept {
            unsubmitted = false;
            acquiredImageWithoutSubmission = false;
        }

    private:
        void init() override;

        void destroy() override;
    };

    using Fence = std::unique_ptr<FenceImpl>;
    using FencePtr = FenceImpl *;
    using FenceRef = FenceImpl &;
};
