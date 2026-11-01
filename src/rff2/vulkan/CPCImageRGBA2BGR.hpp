//
// Created by Merutilm on 2025-09-10.
// Modified by Opus 5 on 2026-08-10, 2026-08-19
// Modified by GPT-6 on 2026-09-23
// Modified by Opus 5.5 on 2026-10-07, 2026-10-08
//

#pragma once

#include "../../vulkan_helper/configurator/ComputePipelineConfigurator.hpp"
#include "../attr/RndSsaaFilter.h"
#include "../attr/VidHdrTransfer.h"

namespace merutilm::rff2 {
    struct CPCImageRGBA2BGR final : public vkh::ComputePipelineConfigurator {
        static constexpr uint32_t SET_INFO = 0;
        static constexpr uint32_t BINDING_PREV_IMAGE_SAMPLER = 0;
        static constexpr uint32_t BINDING_OUTPUT_SSBO = 1;
        static constexpr uint32_t BINDING_OUTPUT_EXTENT_UBO = 2;
        static constexpr uint32_t TARGET_OUTPUT_SSBO_DATA = 0;
        static constexpr uint32_t TARGET_OUTPUT_EXTENT_UBO_EXTENT = 0;
        static constexpr uint32_t TARGET_OUTPUT_EXTENT_UBO_TRANSFER = 1;
        static constexpr uint32_t TARGET_OUTPUT_EXTENT_UBO_SSAA_FILTER = 2;

        explicit CPCImageRGBA2BGR(vkh::EngineRef engine, const uint32_t windowContextIndex)
            : ComputePipelineConfigurator(engine, windowContextIndex, "vk_image_rgba2bgr.comp") {
        }

        void updateQueue(vkh::DescriptorUpdateQueue &queue, uint32_t frameIndex) override;

        void pipelineInitialized() override;

        void renderContextRefreshed() override;

        // Folds the SSAA box filter in here, so readback, staging memory and CPU averaging all shrink by factor^2.
        void setDownsample(uint32_t factor);

        // Packs rgba64le rather than BGR24, which is what an HDR encoder is fed and what sizes the buffer.
        void setTransfer(VidHdrTransfer outputTransfer);

        // How the samples are combined.
        void setSsaaFilter(RndSsaaFilter filter);

        [[nodiscard]] bool isHdr() const {
            return transfer != VidHdrTransfer::SDR;
        }

        [[nodiscard]] const VkExtent2D &getOutputExtent() const {
            return outputExtent;
        }

        [[nodiscard]] const vkh::BufferContext &getBufferContext(uint32_t frameIndex) const;

    protected:
        void configurePushConstant(vkh::PipelineLayoutManagerRef pipelineLayoutManager) override;

        void configureDescriptors(std::vector<vkh::DescriptorPtr> &descriptors) override;

    private:
        uint32_t downsample = 1;
        VidHdrTransfer transfer = VidHdrTransfer::SDR;
        RndSsaaFilter ssaaFilter = RndSsaaFilter::BOX_GAMMA;
        VkExtent2D outputExtent = {};

        void applyOutputSize();
    };
}
