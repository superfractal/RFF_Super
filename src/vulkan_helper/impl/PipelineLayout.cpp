//
// Created by Merutilm on 2025-07-13.
// Modified by GPT-6 on 2026-09-23
//

#include "PipelineLayout.hpp"

#include <utility>

#include "../core/allocator.hpp"
#include "../core/exception.hpp"

namespace merutilm::vkh {
    PipelineLayoutImpl::PipelineLayoutImpl(CoreRef core,
                                           PipelineLayoutManager &&pipelineLayoutManager)
        : CoreHandler(core),
          builders(std::move(pipelineLayoutManager->builders)),
          descriptorSetLayoutCount(pipelineLayoutManager->descriptorSetLayoutCount) {
        PipelineLayoutImpl::init();
    }

    PipelineLayoutImpl::~PipelineLayoutImpl() {
        PipelineLayoutImpl::destroy();
    }

    void PipelineLayoutImpl::cmdPush(const VkCommandBuffer commandBuffer) const {
        uint32_t offsetBytes = 0;
        for (const auto *pushConstant : getPushConstants()) {
            const uint32_t sizeBytes = pushConstant->getHostObject().getTotalSizeByte();
            vkCmdPushConstants(commandBuffer, layout, pushConstant->getUseStage(),
                             offsetBytes, sizeBytes,
                             pushConstant->getHostObject().getData().data());
            offsetBytes += sizeBytes;
        }
    }

    void PipelineLayoutImpl::init() {
        if (layout != VK_NULL_HANDLE) {
            throw exception_invalid_state("Pipeline layout is already initialized");
        }
        uint32_t offsetBytes = 0;
        std::vector<VkPushConstantRange> pushConstantRanges;
        for (const auto *pushConstant : getPushConstants()) {
            const uint32_t sizeBytes = pushConstant->getHostObject().getTotalSizeByte();
            pushConstantRanges.emplace_back(pushConstant->getUseStage(), offsetBytes, sizeBytes);
            offsetBytes += sizeBytes;
        }

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        for (const auto *descriptorSetLayout : getDescriptorSetLayouts()) {
            descriptorSetLayouts.push_back(descriptorSetLayout->getLayoutHandle());
        }

        const VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
            .pSetLayouts = descriptorSetLayouts.empty() ? nullptr : descriptorSetLayouts.data(),
            .pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size()),
            .pPushConstantRanges = pushConstantRanges.empty() ? nullptr : pushConstantRanges.data(),
        };


        VkPipelineLayout createdLayout = VK_NULL_HANDLE;
        if (allocator::invoke(vkCreatePipelineLayout, core.getLogicalDevice().getLogicalDeviceHandle(),
                              &pipelineLayoutInfo, nullptr, &createdLayout) != VK_SUCCESS) {
            throw exception_init("Failed to create pipeline layout!");
        }
        layout = createdLayout;
    }

    void PipelineLayoutImpl::destroy() {
        if (layout == VK_NULL_HANDLE) {
            return;
        }
        allocator::invoke(vkDestroyPipelineLayout, core.getLogicalDevice().getLogicalDeviceHandle(), layout, nullptr);
        layout = VK_NULL_HANDLE;
    }
}
