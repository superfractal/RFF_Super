//
// Modified by Opus 5.5 on 2026-10-05, 2026-10-09
//

#include "CPCChaosIrregularity.hpp"

#include <glm/glm.hpp>

#include "SharedDescriptorTemplate.hpp"
#include "../../vulkan_helper/util/BarrierUtils.hpp"

namespace merutilm::rff2 {
    void CPCChaosIrregularity::updateQueue(vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
        //noop
    }

    void CPCChaosIrregularity::pipelineInitialized() {
        //noop
    }

    void CPCChaosIrregularity::renderContextRefreshed() {
        //noop
    }

    void CPCChaosIrregularity::cmdFill(const VkCommandBuffer cbh, const uint32_t frameIndex) {
        using namespace SharedDescriptorTemplate;
        auto &iteration = getDescriptor(SET_ITERATION);
        // The iteration buffer's extent is the grid the fog pass indexes, so it is read back here every frame.
        const auto extent = iteration.get<vkh::Uniform>(0, DescIteration::BINDING_UBO_ITERATION_INFO)
                ->getHostObject().get<glm::uvec2>(DescIteration::TARGET_UBO_ITERATION_EXTENT);
        setExtent({extent.x, extent.y});
        const auto &iterations = iteration.get<vkh::ShaderStorage>(0, DescIteration::BINDING_SSBO_ITERATION_MATRIX)
                ->getBufferContext();
        const auto &cache = iteration.get<vkh::ShaderStorage>(0, DescIteration::BINDING_SSBO_CHAOS_CACHE)
                ->getBufferContext();
        vkh::BarrierUtils::cmdBufferMemoryBarrier(cbh, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
                                                  VK_ACCESS_SHADER_READ_BIT, iterations.buffer, 0, VK_WHOLE_SIZE,
                                                  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
                                                  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        vkh::BarrierUtils::cmdBufferMemoryBarrier(cbh, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                                                  cache.buffer, 0, VK_WHOLE_SIZE,
                                                  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                                  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        auto &stage = getPushConstant(0).getHostObject();
        stage.set<uint32_t>(PUSH_STAGE, STAGE_IRREGULARITY);
        cmdRender(cbh, frameIndex, {});
        const auto &fog = getDescriptor(SET_FOG).get<vkh::Uniform>(0, DescFog::BINDING_UBO_FOG)->getHostObject();
        const bool edges = fog.get<float>(DescFog::TARGET_FOG_CHAOS_AMOUNT) > 0.0f &&
                           fog.get<float>(DescFog::TARGET_FOG_CHAOS_EDGES) > 0.5f;
        if (edges) {
            // The defocus feathers the irregularity of its neighbours, so all of stage 0 has to land first.
            const auto &defocus = iteration.get<vkh::ShaderStorage>(0, DescIteration::BINDING_SSBO_CHAOS_DEFOCUS)
                    ->getBufferContext();
            vkh::BarrierUtils::cmdBufferMemoryBarrier(cbh, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                                                      cache.buffer, 0, VK_WHOLE_SIZE,
                                                      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
            vkh::BarrierUtils::cmdBufferMemoryBarrier(cbh, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                                                      defocus.buffer, 0, VK_WHOLE_SIZE,
                                                      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                                      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
            stage.set<uint32_t>(PUSH_STAGE, STAGE_DEFOCUS);
            cmdRender(cbh, frameIndex, {});
            vkh::BarrierUtils::cmdBufferMemoryBarrier(cbh, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                                                      defocus.buffer, 0, VK_WHOLE_SIZE,
                                                      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        }
        vkh::BarrierUtils::cmdBufferMemoryBarrier(cbh, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                                                  cache.buffer, 0, VK_WHOLE_SIZE,
                                                  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }

    void CPCChaosIrregularity::configurePushConstant(vkh::PipelineLayoutManagerRef pipelineLayoutManager) {
        auto manager = vkh::factory::create<vkh::HostDataObjectManager>();
        manager->reserve<uint32_t>(PUSH_STAGE);
        push = vkh::factory::create<vkh::PushConstant>(VK_SHADER_STAGE_COMPUTE_BIT, std::move(manager));
        push->getHostObject().set<uint32_t>(PUSH_STAGE, STAGE_IRREGULARITY);
        pipelineLayoutManager.appendPushConstantManager(push.get());
    }

    void CPCChaosIrregularity::configureDescriptors(std::vector<vkh::DescriptorPtr> &descriptors) {
        using namespace SharedDescriptorTemplate;
        appendDescriptor<DescIteration>(SET_ITERATION, descriptors);
        appendDescriptor<DescFog>(SET_FOG, descriptors);
    }
}
