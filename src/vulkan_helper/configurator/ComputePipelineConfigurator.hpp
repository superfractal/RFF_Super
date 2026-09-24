//
// Created by Merutilm on 2025-08-28.
// Modified by GPT-5 on 2026-08-23
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include <cstdint>
#include <string>

#include "PipelineConfigurator.hpp"

namespace merutilm::vkh {
    class ComputePipelineConfigurator : public PipelineConfiguratorAbstract {
        ShaderModuleRef computeShader;
        VkPipelineCreateFlags pipelineCreateFlags;
        VkExtent2D extent = {};
        static constexpr uint32_t WORK_GROUP_SIZE = 16;

    public:
        explicit ComputePipelineConfigurator(EngineRef engine, const uint32_t windowContextIndex,
                                             const std::string &computeShaderName,
                                             const VkPipelineCreateFlags pipelineCreateFlags = 0)
            : PipelineConfiguratorAbstract(engine, windowContextIndex),
              computeShader(pickFromGlobalRepository<GlobalShaderModuleRepo, ShaderModuleRef>(computeShaderName)),
              pipelineCreateFlags(pipelineCreateFlags) {
        }

        ~ComputePipelineConfigurator() override = default;

        ComputePipelineConfigurator(const ComputePipelineConfigurator &) = delete;

        ComputePipelineConfigurator &operator=(const ComputePipelineConfigurator &) = delete;

        ComputePipelineConfigurator(ComputePipelineConfigurator &&) = delete;

        ComputePipelineConfigurator &operator=(ComputePipelineConfigurator &&) = delete;

        void cmdRender(VkCommandBuffer commandBuffer, uint32_t frameIndex,
                       DescIndexPicker &&descIndices) override;

        void setExtent(const VkExtent2D &extent) {
            this->extent = extent;
        }

        void configure() override;

    protected:
        void cmdDispatch(const VkCommandBuffer commandBuffer) const {
            const auto [width, height] = extent;
            vkCmdDispatch(commandBuffer,
                          (width + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE,
                          (height + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE, 1);
        }
    };
}
