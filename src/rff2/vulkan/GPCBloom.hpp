//
// Created by Merutilm on 2025-08-30.
// Modified by GPT-5 on 2026-08-18
// Modified by Opus 5 on 2026-08-19
//

#pragma once
#include "../../vulkan_helper/configurator/GeneralPostProcessGraphicsPipelineConfigurator.hpp"
#include "../attr/ShdBloomAttribute.h"
#include "../attr/ShdHdrAttribute.h"

namespace merutilm::rff2 {
    struct GPCBloom final : public vkh::GeneralPostProcessGraphicsPipelineConfigurator {
        static constexpr uint32_t SET_BLOOM_CANVAS = 0;
        static constexpr uint32_t BINDING_BLOOM_CANVAS_ORIGINAL = 0;
        static constexpr uint32_t BINDING_BLOOM_CANVAS_BLURRED = 1;

        static constexpr uint32_t SET_BLOOM = 1;

        explicit GPCBloom(vkh::EngineRef engine, const uint32_t windowContextIndex, const uint32_t renderContextIndex,
                                           const uint32_t
                                           subpassIndex) : GeneralPostProcessGraphicsPipelineConfigurator(
            engine, windowContextIndex, renderContextIndex, subpassIndex, "vk_bloom.frag") {
        }

        void updateQueue(vkh::DescriptorUpdateQueue &queue, uint32_t frameIndex) override;

        // The HDR block rides along because this is the pass that makes light brighter than white,
        // so it is the one that has to know the headroom it is stored against.
        void setBloom(const ShdBloomAttribute &bloom, const ShdHdrAttribute &hdr) const;

        void setBloomDynamic(const ShdBloomAttribute &bloom, const ShdHdrAttribute &hdr) const;

        void pipelineInitialized() override;

        void renderContextRefreshed() override;

    protected:
        void configurePushConstant(vkh::PipelineLayoutManagerRef pipelineLayoutManager) override;


        void configureDescriptors(std::vector<vkh::DescriptorPtr> &descriptors) override;
    };
}
