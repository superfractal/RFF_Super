//
// Created by Merutilm on 2025-08-13.
// Modified by GPT-6 on 2026-09-23
//

#include "Sampler.hpp"

#include "../core/allocator.hpp"

namespace merutilm::vkh {
    SamplerImpl::SamplerImpl(CoreRef core, const VkSamplerCreateInfo &samplerInfo)
        : CoreHandler(core), samplerInfo(samplerInfo) {
        SamplerImpl::init();
    }

    SamplerImpl::~SamplerImpl() {
        SamplerImpl::destroy();
    }

    void SamplerImpl::init() {
        const VkDevice device = core.getLogicalDevice().getLogicalDeviceHandle();
        if (allocator::invoke(vkCreateSampler, device, &samplerInfo, nullptr, &sampler)) {
            throw exception_init("Failed to create sampler!");
        }
    }

    void SamplerImpl::destroy() {
        const VkDevice device = core.getLogicalDevice().getLogicalDeviceHandle();
        allocator::invoke(vkDestroySampler, device, sampler, nullptr);
    }
}
