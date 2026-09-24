//
// Created by Merutilm on 2025-08-15.
// Modified by GPT-6 on 2026-09-11, 2026-09-16, 2026-09-23
//

#pragma once
#include "ShaderLayerControl.hpp"
#include "../../vulkan_helper/configurator/GeneralPostProcessGraphicsPipelineConfigurator.hpp"
#include "../attr/ShdSlopeAttribute.h"
#include "../attr/ShdEffectsAttribute.h"
#include "../attr/ShdPaletteAttribute.h"

namespace merutilm::rff2 {
    struct GPCSlope final : public vkh::GeneralPostProcessGraphicsPipelineConfigurator {
        vkh::PushConstant layerPush;
        static constexpr uint32_t SET_PREV_RESULT = 0;
        static constexpr uint32_t BINDING_PREV_RESULT_SAMPLER = 0;

        static constexpr uint32_t SET_ITERATION = 1;
        static constexpr uint32_t SET_SLOPE = 2;
        static constexpr uint32_t SET_EFFECTS = 3;
        static constexpr uint32_t SET_EFFECTS_TIME = 4;
        static constexpr uint32_t SET_PALETTE = 5;
        static constexpr uint32_t SET_PALETTE_TIME = 7;
        vkh::DescriptorPtr paletteTextures;

        explicit GPCSlope(vkh::EngineRef engine, const uint32_t windowContextIndex,
                                  const uint32_t renderContextIndex,
                                  const uint32_t primarySubpassIndex, vkh::DescriptorRef textures) : GeneralPostProcessGraphicsPipelineConfigurator(
            engine, windowContextIndex, renderContextIndex, primarySubpassIndex, "vk_slope.frag"), paletteTextures(&textures) {
        }

        void updateQueue(vkh::DescriptorUpdateQueue &queue, uint32_t frameIndex) override;

        void setEffects(const ShdEffectsAttribute &effects, bool sceneLinear, bool hasIterations = true) const;

        void setSlope(const ShdSlopeAttribute &slope) const;
        void setGroove(const ShdPaletteAttribute &palette) const;
        void setReliefZoom(const ShdSlopeAttribute &slope, float logZoom) const;

        void pipelineInitialized() override;

        void renderContextRefreshed() override;

    protected:
        void configurePushConstant(vkh::PipelineLayoutManagerRef pipelineLayoutManager) override;

        void configureDescriptors(std::vector<vkh::DescriptorPtr> &descriptors) override;
    };
}
