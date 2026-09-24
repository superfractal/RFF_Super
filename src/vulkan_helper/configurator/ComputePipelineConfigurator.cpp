//
// Created by Merutilm on 2025-08-28.
// Modified by GPT-5 on 2026-08-23
// Modified by Fable 5.1 on 2026-09-06
// Modified by GPT-6 on 2026-09-17, 2026-09-23
//

#include <utility>
#include <vector>

#include "ComputePipelineConfigurator.hpp"
#include "../impl/ComputeShaderPipeline.hpp"
#include "../repo/GlobalPipelineLayoutRepo.hpp"

namespace merutilm::vkh {
    void ComputePipelineConfigurator::cmdRender(const VkCommandBuffer commandBuffer,
                                                const uint32_t frameIndex,
                                                DescIndexPicker &&descIndices) {
        pipeline->cmdBindAll(commandBuffer, frameIndex, std::move(descIndices));
        pipeline->getLayout().cmdPush(commandBuffer);
        cmdDispatch(commandBuffer);
    }

    void ComputePipelineConfigurator::configure() {
        auto pipelineLayoutManager = factory::create<PipelineLayoutManager>();

        std::vector<DescriptorPtr> descriptors;
        configureDescriptors(descriptors);

        for (const auto descriptor : descriptors) {
            pipelineLayoutManager->appendDescriptorSetLayout(&descriptor->getLayout());
        }

        configurePushConstant(*pipelineLayoutManager);
        PipelineLayoutRef pipelineLayout =
            pickFromGlobalRepository<GlobalPipelineLayoutRepo, PipelineLayoutRef>(
                std::move(pipelineLayoutManager));

        auto pipelineManager = factory::create<PipelineManager>(pipelineLayout);

        pipelineManager->attachDescriptor(std::move(descriptors));
        pipelineManager->attachShader(&computeShader);
        pipelineManager->attachSpecialization(specializationConstants());

        pipeline = factory::create<ComputeShaderPipeline>(
            wc, pipelineLayout, std::move(pipelineManager), pipelineCreateFlags);
    }
}
