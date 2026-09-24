//
// Created by Merutilm on 2025-08-05.
// Modified by Fable 5.1 on 2026-09-06
// Modified by GPT-6 on 2026-09-23
//

#include "GeneralPostProcessGraphicsPipelineConfigurator.hpp"

#include "../core/vkh_core.hpp"
#include "../impl/GraphicsPipeline.hpp"
#include "../repo/GlobalPipelineLayoutRepo.hpp"
#include "../struct/Vertex.hpp"

namespace merutilm::vkh {
    void GeneralPostProcessGraphicsPipelineConfigurator::cmdRender(const VkCommandBuffer cbh, const uint32_t frameIndex,
                                                                   DescIndexPicker &&descIndices) {
        pipeline->cmdBindAll(cbh, frameIndex, std::move(descIndices));
        cmdPushAll(cbh);
        cmdDraw(cbh, frameIndex, 0);
    }


    void GeneralPostProcessGraphicsPipelineConfigurator::configureVertexBuffer(HostDataObjectManagerRef vertexData) {
        vertexData.addArray(0, std::vector{
                                Vertex::generate({1, 1, 0}, {1, 1, 1}, {1, 1}),
                                Vertex::generate({1, -1, 0}, {1, 1, 1}, {1, 0}),
                                Vertex::generate({-1, -1, 0}, {1, 1, 1}, {0, 0}),
                                Vertex::generate({-1, 1, 0}, {1, 1, 1}, {0, 1}),
                            });
    }

    void GeneralPostProcessGraphicsPipelineConfigurator::configureIndexBuffer(HostDataObjectManagerRef indexData) {
        indexData.addArray(0, std::vector<uint32_t>{0, 1, 2, 2, 3, 0});
    }

    void GeneralPostProcessGraphicsPipelineConfigurator::configure() {
        auto pipelineLayoutManager = factory::create<PipelineLayoutManager>();

        std::vector<DescriptorPtr> descriptors;
        configureDescriptors(descriptors);

        for (const auto descriptor : descriptors) {
            pipelineLayoutManager->appendDescriptorSetLayout(&descriptor->getLayout());
        }

        configurePushConstant(*pipelineLayoutManager);
        PipelineLayoutRef pipelineLayout = engine.getGlobalRepositories()
            .getRepository<GlobalPipelineLayoutRepo>()->pick(std::move(pipelineLayoutManager));

        auto pipelineManager = factory::create<PipelineManager>(pipelineLayout);

        pipelineManager->attachDescriptor(std::move(descriptors));
        pipelineManager->attachShader(&vertexShader);
        pipelineManager->attachShader(&fragmentShader);
        pipelineManager->attachSpecialization(specializationConstants());
        if (!initializedVertexIndex) {
            auto vertexData = factory::create<HostDataObjectManager>();
            auto indexData = factory::create<HostDataObjectManager>();

            configureVertexBuffer(*vertexData);
            configureIndexBuffer(*indexData);

            vertexBufferPP = factory::create<VertexBuffer>(wc.core, std::move(vertexData),
                                                           BufferLock::LOCK_ONLY, false);
            indexBufferPP = factory::create<IndexBuffer>(wc.core, std::move(indexData),
                                                         BufferLock::LOCK_ONLY, false);
            vertexBufferPP->update();
            indexBufferPP->update();
            vertexBufferPP->lock(wc.getCommandPool());
            indexBufferPP->lock(wc.getCommandPool());
            initializedVertexIndex = true;
        }

        pipeline = factory::create<GraphicsPipeline>(wc, pipelineLayout, getVertexBuffer(), getIndexBuffer(),
                                                     renderContextIndex, primarySubpassIndex,
                                                     std::move(pipelineManager));
    }
}
