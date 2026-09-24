//
// Created by Merutilm on 2025-08-27.
// Modified by GPT-5 on 2026-08-23
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include "Pipeline.hpp"
#include "../handle/WindowContextHandler.hpp"
#include "../manage/PipelineManager.hpp"

namespace merutilm::vkh {
    class ComputeShaderPipelineImpl final : public PipelineAbstract {
        VkPipelineCreateFlags pipelineCreateFlags;

    public:
        explicit ComputeShaderPipelineImpl(WindowContextRef wc, PipelineLayoutRef pipelineLayout,
                                           PipelineManager &&pipelineManager,
                                           VkPipelineCreateFlags pipelineCreateFlags = 0);

        ~ComputeShaderPipelineImpl() override;

        ComputeShaderPipelineImpl(const ComputeShaderPipelineImpl &) = delete;

        ComputeShaderPipelineImpl &operator=(const ComputeShaderPipelineImpl &) = delete;

        ComputeShaderPipelineImpl(ComputeShaderPipelineImpl &&) = delete;

        ComputeShaderPipelineImpl &operator=(ComputeShaderPipelineImpl &&) = delete;

        void cmdBindAll(VkCommandBuffer commandBuffer, uint32_t frameIndex,
                        DescIndexPicker &&descriptorIndices) const override;

        void init() override;
    };

    using ComputeShaderPipeline = std::unique_ptr<ComputeShaderPipelineImpl>;
}
