//
// Created by Merutilm on 2025-07-13.
// Modified by GPT-6 on 2026-09-23
//

#include "DescriptorSetLayout.hpp"
#include "../core/exception.hpp"

namespace merutilm::vkh {
    DescriptorSetLayoutImpl::DescriptorSetLayoutImpl(CoreRef core,
                                                     const DescriptorSetLayoutBuilder &layoutBuilder)
        : CoreHandler(core), layoutBuilder(layoutBuilder) {
        DescriptorSetLayoutImpl::init();
    }

    DescriptorSetLayoutImpl::~DescriptorSetLayoutImpl() {
        DescriptorSetLayoutImpl::destroy();
    }

    void DescriptorSetLayoutImpl::init() {
        if (layout != nullptr) {
            throw exception_invalid_state("double-finish called");
        }

        std::vector<VkDescriptorSetLayoutBinding> bindings(layoutBuilder.size());
        for (uint32_t i = 0; i < layoutBuilder.size(); ++i) {
            const auto &descriptor = layoutBuilder[i];
            bindings[i] = {
                .binding = i,
                .descriptorType = descriptor.type,
                .descriptorCount = 1,
                .stageFlags = descriptor.stage,
                .pImmutableSamplers = nullptr
            };
        }

        const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.empty() ? nullptr : bindings.data(),
        };
        const auto device = core.getLogicalDevice().getLogicalDeviceHandle();
        if (allocator::invoke(vkCreateDescriptorSetLayout, device, &descriptorSetLayoutInfo,
                              nullptr, &layout) != VK_SUCCESS) {
            throw exception_init("Failed to create descriptor set layout");
        }
    }

    void DescriptorSetLayoutImpl::destroy() {
        if (layout != nullptr) {
            const auto device = core.getLogicalDevice().getLogicalDeviceHandle();
            allocator::invoke(vkDestroyDescriptorSetLayout, device, layout, nullptr);
        }
    }
}
