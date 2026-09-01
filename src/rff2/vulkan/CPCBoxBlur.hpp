//
// Created by Merutilm on 2025-08-28.
//
// Modified by Opus 5 on 2026-08-19, 2026-08-24
//

#pragma once
#include "../../vulkan_helper/configurator/ComputePipelineConfigurator.hpp"

namespace merutilm::rff2 {
    struct CPCBoxBlur final : public vkh::ComputePipelineConfigurator {
        static constexpr uint32_t SET_BLUR_IMAGE = 0;

        static constexpr uint32_t BINDING_BLUR_IMAGE_SRC = 0;
        static constexpr uint32_t BINDING_BLUR_IMAGE_DST = 1;

        static constexpr uint32_t SET_BLUR_RADIUS = 1;
        static constexpr uint32_t BINDING_BLUR_RADIUS_UBO = 0;

        static constexpr uint32_t TARGET_BLUR_UBO_BLUR_SIZE = 0;
        static constexpr uint32_t TARGET_BLUR_UBO_AXIS = 1;

        static constexpr uint32_t DESC_INDEX_BLUR_TARGET_FOG = 0;
        static constexpr uint32_t DESC_INDEX_BLUR_TARGET_BLOOM = 1;
        // One radius block per target per axis, and a last one left at radius zero for the copy below.
        static constexpr uint32_t DESC_COUNT_BLUR_TARGET = 5;
        static constexpr uint32_t DESC_INDEX_BLUR_COPY = 4;

        static constexpr uint32_t BOX_BLUR_COUNT = 3;
        // Two axes per box, plus the copy that carries an even number of passes back to the destination.
        static constexpr uint32_t BOX_BLUR_PASS_COUNT = BOX_BLUR_COUNT * 2 + 1;

        explicit CPCBoxBlur(vkh::EngineRef engine, const uint32_t windowContextIndex) : ComputePipelineConfigurator(
            engine, windowContextIndex, "vk_box_blur.comp") {
        }

        ~CPCBoxBlur() override = default;

        CPCBoxBlur(const CPCBoxBlur &) = delete;

        CPCBoxBlur &operator=(const CPCBoxBlur &) = delete;

        CPCBoxBlur(CPCBoxBlur &&) = delete;

        CPCBoxBlur &operator=(CPCBoxBlur &&) = delete;


        void updateQueue(vkh::DescriptorUpdateQueue &queue, uint32_t frameIndex) override;

        void setGaussianBlur(
            const vkh::MultiframeImageContext &srcImage, const vkh::MultiframeImageContext &dstImage);

        void cmdGaussianBlur(
            uint32_t frameIndex, uint32_t blurTargetDescIndex);

        void setImages(uint32_t descIndex, const vkh::MultiframeImageContext &srcImage,
                       const vkh::MultiframeImageContext &dstImage) const;

        void setBlurInfo(uint32_t blurTargetDescIndex, float blurSize) const;

        // Which radius block a pass reads: one per axis, and the copy block for the last pass.
        static uint32_t radiusDescIndex(uint32_t blurTargetDescIndex, uint32_t pass);

        void configure() override {
            ComputePipelineConfigurator::configure();
            initSize();
        }

        void pipelineInitialized() override;

        void renderContextRefreshed() override;

    protected:
        void initSize() const;

        void configurePushConstant(vkh::PipelineLayoutManagerRef pipelineLayoutManager) override;

        void configureDescriptors(std::vector<vkh::DescriptorPtr> &descriptors) override;
    };
}
