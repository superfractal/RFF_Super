//
// Created by Merutilm on 2025-08-27.
// Modified by Opus 5 on 2026-08-23
// Modified by GPT-5 on 2026-08-23.
//

#include "ComputeShaderPipeline.hpp"

namespace merutilm::vkh {
    ComputeShaderPipelineImpl::ComputeShaderPipelineImpl(WindowContextRef wc, PipelineLayoutRef pipelineLayout,
                                                         PipelineManager &&pipelineManager,
                                                         const VkPipelineCreateFlags pipelineCreateFlags) : PipelineAbstract(
        wc, pipelineLayout, std::move(pipelineManager)), pipelineCreateFlags(pipelineCreateFlags) {
        ComputeShaderPipelineImpl::init();
    }

    ComputeShaderPipelineImpl::~ComputeShaderPipelineImpl() {
        ComputeShaderPipelineImpl::destroy();
    }


    void ComputeShaderPipelineImpl::cmdBindAll(const VkCommandBuffer cbh, const uint32_t frameIndex, DescIndexPicker &&descIndices) const {
        const auto sets = enumerateDescriptorSets(frameIndex, std::move(descIndices));
        vkCmdBindPipeline(cbh, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        vkCmdBindDescriptorSets(cbh, VK_PIPELINE_BIND_POINT_COMPUTE,
                                getLayout().getLayoutHandle(), 0,
                                static_cast<uint32_t>(sets.size()), sets.data(), 0,
                                nullptr);
    }


    void ComputeShaderPipelineImpl::init() {
        const VkComputePipelineCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = pipelineCreateFlags,
            .stage = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                .module = getShaderModules()[0]->getShaderModuleHandle(),
                .pName = "main",
                .pSpecializationInfo = nullptr
            },
            .layout = pipelineLayout.getLayoutHandle(),
            .basePipelineHandle = nullptr,
            .basePipelineIndex = -1
        };

        if (allocator::invoke(vkCreateComputePipelines, wc.core.getLogicalDevice().getLogicalDeviceHandle(),
                                     wc.core.getLogicalDevice().getPipelineCacheHandle(), 1, &info,
                                     nullptr, &pipeline) != VK_SUCCESS) {
            throw exception_init("Failed to create compute pipeline!");
        }
    }

}
