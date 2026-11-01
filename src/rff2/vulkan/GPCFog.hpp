//
// Created by Merutilm on 2025-08-15.
// Modified by Opus 5 on 2026-08-07
// Modified by GPT-6 on 2026-09-11, 2026-09-16, 2026-09-23
// Modified by Opus 5.5 on 2026-10-09
//

#pragma once

#include "ShaderLayerControl.hpp"
#include "../../vulkan_helper/configurator/GeneralPostProcessGraphicsPipelineConfigurator.hpp"
#include "../attr/ShdFogAttribute.h"

namespace merutilm::rff2 {
    struct GPCFog final : public vkh::GeneralPostProcessGraphicsPipelineConfigurator {
        vkh::PushConstant layerPush;
        static constexpr uint32_t SET_FOG_CANVAS = 0;
        static constexpr uint32_t BINDING_FOG_CANVAS_ORIGINAL = 0;
        static constexpr uint32_t BINDING_FOG_CANVAS_BLURRED = 1;

        static constexpr uint32_t SET_FOG = 1;
        // The rim mask rebuilds the slope pass's rim footprint, so fog needs the same iteration
        // buffer and slope settings that vk_slope.frag reads.
        static constexpr uint32_t SET_ITERATION = 2;
        static constexpr uint32_t SET_SLOPE = 3;

    private:
        // The value last written, so a frame that does not move it skips the upload.
        float focusFrame = 0.0f;

    public:

        explicit GPCFog(vkh::EngineRef engine,
                        const uint32_t windowContextIndex,
                        const uint32_t renderContextIndex,
                        const uint32_t subpassIndex)
            : GeneralPostProcessGraphicsPipelineConfigurator(
                  engine, windowContextIndex, renderContextIndex, subpassIndex, "vk_fog.frag") {
        }

        ~GPCFog() override = default;

        GPCFog(const GPCFog &) = delete;

        GPCFog(GPCFog &&) = delete;

        GPCFog &operator=(const GPCFog &) = delete;

        GPCFog &operator=(GPCFog &&) = delete;

        void updateQueue(vkh::DescriptorUpdateQueue &queue, uint32_t frameIndex) override;

        void setFog(const ShdFogAttribute &fog) const;

        // The keyframe depth a video frame is drawn at, which Smooth Focus Across Keyframes reads its
        // position between the two keyframes from. Left at 0 outside a video; setFog() leaves it alone.
        void setFocusFrame(float frame);

        void pipelineInitialized() override;

        void renderContextRefreshed() override;

    protected:
        void configurePushConstant(vkh::PipelineLayoutManagerRef pipelineLayoutManager) override;

        void configureDescriptors(std::vector<vkh::DescriptorPtr> &descriptors) override;
    };
}
