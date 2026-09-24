//
// Created by Merutilm on 2025-09-05.
// Modified by GPT-6 on 2026-09-18, 2026-09-23
//

#pragma once

#include "../../vulkan_helper/configurator/GeneralPostProcessGraphicsPipelineConfigurator.hpp"

namespace merutilm::rff2 {
    struct GPCPresent final : public vkh::GeneralPostProcessGraphicsPipelineConfigurator {
        vkh::PushConstant zoomPush;
        int heldFrame = -1;
        static constexpr uint32_t SET_PRESENT = 0;

        static constexpr uint32_t BINDING_PRESENT_SAMPLER = 0;
        static constexpr uint32_t BINDING_PRESENT_UBO = 1;

        static constexpr uint32_t TARGET_PRESENT_UBO_EXTENT = 0;
        static constexpr uint32_t TARGET_PRESENT_UBO_SRGB = 1;

        explicit GPCPresent(vkh::EngineRef engine,
                            const uint32_t windowContextIndex,
                            const uint32_t renderContextIndex,
                            const uint32_t primarySubpassIndex)
            : GeneralPostProcessGraphicsPipelineConfigurator(
                  engine, windowContextIndex, renderContextIndex, primarySubpassIndex, "vk_smooth_present.frag") {
        }

        ~GPCPresent() override = default;

        GPCPresent(const GPCPresent &) = delete;

        GPCPresent(GPCPresent &&) = delete;

        GPCPresent &operator=(const GPCPresent &) = delete;

        GPCPresent &operator=(GPCPresent &&) = delete;

        void updateQueue(vkh::DescriptorUpdateQueue &queue, uint32_t frameIndex) override;

        void setRescaledResolution(const glm::uvec2 &newResolution) const;

        void setZoomTransform(float scale, float offsetX, float offsetY) const {
            zoomPush->getHostObject().set(0, glm::vec4(scale, offsetX, offsetY, 0.0f));
        }

        void pipelineInitialized() override;

        void renderContextRefreshed() override;

    protected:
        void configurePushConstant(vkh::PipelineLayoutManagerRef pipelineLayoutManager) override;

        void configureDescriptors(std::vector<vkh::DescriptorPtr> &descriptors) override;
    };
}
