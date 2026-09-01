//
// Created by Merutilm on 2025-08-31.
// Modified by Opus 5 on 2026-08-15, 2026-08-19, 2026-08-31
//

#pragma once
#include "../../vulkan_helper/configurator/GeneralPostProcessGraphicsPipelineConfigurator.hpp"
#include "../attr/ShdHdrAttribute.h"
#include "../attr/VidHdrTransfer.h"

namespace merutilm::rff2 {
    struct GPCLinearInterpolation final : public vkh::GeneralPostProcessGraphicsPipelineConfigurator {
        static constexpr uint32_t SET_PREV_RESULT = 0;
        static constexpr uint32_t BINDING_PREV_RESULT_SAMPLER = 0;

        static constexpr uint32_t SET_LINEAR_INTERPOLATION = 1;

        explicit GPCLinearInterpolation(vkh::EngineRef engine, const uint32_t windowContextIndex, const uint32_t renderContextIndex,
                                                         const uint32_t subpassIndex) : GeneralPostProcessGraphicsPipelineConfigurator(
            engine, windowContextIndex, renderContextIndex, subpassIndex, "vk_linear_interpolation.frag") {
        }


        void updateQueue(vkh::DescriptorUpdateQueue &queue, uint32_t frameIndex) override;

        void setLinearInterpolation(bool use) const;

        void setDither(bool use) const;

        // The last pass owns the output transform, so it is also the only one that knows what the frame is for.
        void setToneMap(const ShdHdrAttribute &hdr, VidHdrTransfer transfer, float peakNits) const;

        void pipelineInitialized() override;

        void renderContextRefreshed() override;

    protected:
        void configurePushConstant(vkh::PipelineLayoutManagerRef pipelineLayoutManager) override;

        void configureDescriptors(std::vector<vkh::DescriptorPtr> &descriptors) override;
    };
}
