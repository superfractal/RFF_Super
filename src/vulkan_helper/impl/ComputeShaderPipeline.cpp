//
// Created by Merutilm on 2025-08-27.
// Modified by Opus 5 on 2026-08-23
// Modified by GPT-5 on 2026-08-23
// Modified by Fable 5.1 on 2026-09-06
// Modified by GPT-6 on 2026-09-18, 2026-09-20, 2026-09-23
//

#include "ComputeShaderPipeline.hpp"
#include "PipelinePreparation.hpp"
#include "../core/allocator.hpp"
#include "../core/exception.hpp"

namespace merutilm::vkh {
    ComputeShaderPipelineImpl::ComputeShaderPipelineImpl(WindowContextRef wc, PipelineLayoutRef pipelineLayout,
                                                         PipelineManager &&pipelineManager,
                                                         const VkPipelineCreateFlags pipelineCreateFlags) : PipelineAbstract(
        wc, pipelineLayout, std::move(pipelineManager)), pipelineCreateFlags(pipelineCreateFlags) {
        prepare([this] { ComputeShaderPipelineImpl::init(); });
    }

    ComputeShaderPipelineImpl::~ComputeShaderPipelineImpl() {
        waitPreparation();
        ComputeShaderPipelineImpl::destroy();
    }


    void ComputeShaderPipelineImpl::cmdBindAll(const VkCommandBuffer commandBuffer, const uint32_t frameIndex,
                                               DescIndexPicker &&descriptorIndices) const {
        const auto descriptorSets = enumerateDescriptorSets(frameIndex, std::move(descriptorIndices));
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                getLayout().getLayoutHandle(), 0,
                                static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0,
                                nullptr);
    }


    void ComputeShaderPipelineImpl::init() {
        if (pipeline != VK_NULL_HANDLE) {
            throw exception_invalid_state("Compute pipeline is already initialized");
        }

        const auto &computeShader = *getShaderModules().front();
        const VkSpecializationInfo *specialization = getSpecializationInfo();
        const VkPipelineShaderStageCreateInfo shaderStage = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = computeShader.getShaderModuleHandle(),
            .pName = "main",
            .pSpecializationInfo = specialization
        };
        const VkComputePipelineCreateInfo pipelineInfo = {
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = pipelineCreateFlags,
            .stage = shaderStage,
            .layout = pipelineLayout.getLayoutHandle(),
            .basePipelineHandle = nullptr,
            .basePipelineIndex = -1
        };

        const auto &logicalDevice = wc.core.getLogicalDevice();
        const VkDevice deviceHandle = logicalDevice.getLogicalDeviceHandle();
        const VkPipelineCache pipelineCache = logicalDevice.getPipelineCacheHandle();
        VkPipeline createdPipeline = VK_NULL_HANDLE;
        const auto createPipeline = [&] {
            return vkCreateComputePipelines(deviceHandle, pipelineCache, 1, &pipelineInfo, nullptr,
                                            &createdPipeline);
        };
        const auto destroyCreatedPipeline = [&] {
            if (createdPipeline != VK_NULL_HANDLE) {
                allocator::invoke(vkDestroyPipeline, deviceHandle, createdPipeline, nullptr);
                createdPipeline = VK_NULL_HANDLE;
            }
        };
        const auto &deviceProperties = wc.core.getPhysicalDevice().getPhysicalDeviceProperties();
        const std::string deviceIdentifier = std::to_string(deviceProperties.vendorID) + ":" +
                                             std::to_string(deviceProperties.deviceID) + ":" +
                                             std::to_string(deviceProperties.driverVersion);
        VkResult result;
        try {
            result = PipelinePreparation::run(wc.getWindow().getWindowHandle(),
                                              computeShader.getFilename(), deviceIdentifier,
                                              createPipeline);
        } catch (...) {
            destroyCreatedPipeline();
            throw;
        }
        if (result != VK_SUCCESS) {
            destroyCreatedPipeline();
            throw exception_init("Failed to create compute pipeline!");
        }
        pipeline = createdPipeline;
    }

}
