//
// Modified by Opus 5.5 on 2026-10-05, 2026-10-09
//

#pragma once

#include "../../vulkan_helper/configurator/ComputePipelineConfigurator.hpp"

namespace merutilm::rff2 {
    // Fills the per-pixel chaos irregularity the fog pass's defocus gathers, so each pixel is derived once instead of once per tap.
    struct CPCChaosIrregularity final : public vkh::ComputePipelineConfigurator {
        static constexpr uint32_t SET_ITERATION = 0;
        static constexpr uint32_t SET_FOG = 1;
        static constexpr uint32_t PUSH_STAGE = 0;
        // Push values for the shader's two stages: the irregularity fill, then the defocus it feathers into for Chaos Edge Protection.
        static constexpr uint32_t STAGE_IRREGULARITY = 0;
        static constexpr uint32_t STAGE_DEFOCUS = 1;

        explicit CPCChaosIrregularity(vkh::EngineRef engine, const uint32_t windowContextIndex)
            : ComputePipelineConfigurator(engine, windowContextIndex, "vk_chaos_irregularity.comp") {
        }

        ~CPCChaosIrregularity() override = default;

        CPCChaosIrregularity(const CPCChaosIrregularity &) = delete;

        CPCChaosIrregularity &operator=(const CPCChaosIrregularity &) = delete;

        CPCChaosIrregularity(CPCChaosIrregularity &&) = delete;

        CPCChaosIrregularity &operator=(CPCChaosIrregularity &&) = delete;

        void updateQueue(vkh::DescriptorUpdateQueue &queue, uint32_t frameIndex) override;

        void pipelineInitialized() override;

        void renderContextRefreshed() override;

        // Records the fill after the iteration buffer's writes, and orders the fog pass's reads after it; the defocus stage follows only while Chaos Edge Protection is on.
        void cmdFill(VkCommandBuffer cbh, uint32_t frameIndex);

    protected:
        void configurePushConstant(vkh::PipelineLayoutManagerRef pipelineLayoutManager) override;

        void configureDescriptors(std::vector<vkh::DescriptorPtr> &descriptors) override;

    private:
        vkh::PushConstant push = nullptr;
    };
}
