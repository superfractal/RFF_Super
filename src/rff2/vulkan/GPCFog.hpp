//
// Created by Merutilm on 2025-08-15.
//
// Modified by Opus 5 on 2026-08-07
//

#pragma once
#include "../../vulkan_helper/configurator/GeneralPostProcessGraphicsPipelineConfigurator.hpp"
#include "../attr/ShdFogAttribute.h"

namespace merutilm::rff2 {
    struct GPCFog final : public vkh::GeneralPostProcessGraphicsPipelineConfigurator {
        static constexpr uint32_t SET_FOG_CANVAS = 0;
        static constexpr uint32_t BINDING_FOG_CANVAS_ORIGINAL = 0;
        static constexpr uint32_t BINDING_FOG_CANVAS_BLURRED = 1;

        static constexpr uint32_t SET_FOG = 1;
        // The rim mask rebuilds the slope pass's rim footprint, so fog needs the same iteration
        // buffer and slope settings that vk_slope.frag reads.
        static constexpr uint32_t SET_ITERATION = 2;
        static constexpr uint32_t SET_SLOPE = 3;

        explicit GPCFog(vkh::EngineRef engine, const uint32_t windowContextIndex, const uint32_t renderContextIndex,
                        const uint32_t subpassIndex) : GeneralPostProcessGraphicsPipelineConfigurator(
            engine, windowContextIndex, renderContextIndex, subpassIndex, "vk_fog.frag") {
        }

        ~GPCFog() override = default;

        GPCFog(const GPCFog &) = delete;

        GPCFog(GPCFog &&) = delete;

        GPCFog &operator=(const GPCFog &) = delete;

        GPCFog &operator=(GPCFog &&) = delete;

        void updateQueue(vkh::DescriptorUpdateQueue &queue, uint32_t frameIndex) override;

        void setFog(const ShdFogAttribute &fog) const;

        void pipelineInitialized() override;

        void renderContextRefreshed() override;

    protected:
        void configurePushConstant(vkh::PipelineLayoutManagerRef pipelineLayoutManager) override;

        void configureDescriptors(std::vector<vkh::DescriptorPtr> &descriptors) override;
    };
}
