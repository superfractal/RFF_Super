//
// Created by Merutilm on 2025-09-06.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-07-09, 2026-08-21, 2026-08-23.
// Modified by Opus 5 on 2026-08-05, 2026-08-07, 2026-08-13, 2026-08-15, 2026-08-17, 2026-08-18, 2026-08-19, 2026-08-24, 2026-08-25, 2026-08-26
//

#pragma once
#include <array>
#include <string>

#include "ShaderAnimationPhases.hpp"
#include "../../vulkan_helper/configurator/ComputePipelineConfigurator.hpp"
#include "../attr/ShdPaletteAttribute.h"
#include "../attr/ShdPatternAttribute.h"
#include "../attr/ShdStripeAttribute.h"
#include "../attr/ShdTextureAttribute.h"
#include "../attr/ShdWarpAttribute.h"

namespace merutilm::rff2 {
    struct CPC2MapIterationStripe final : public vkh::ComputePipelineConfigurator {

        static constexpr uint32_t SET_I2MAP = 0;
        static constexpr uint32_t BINDING_I2MAP_SSBO_NORMAL = 0;
        static constexpr uint32_t TARGET_I2MAP_SSBO_NORMAL_ITERATION = 0;
        static constexpr uint32_t BINDING_I2MAP_SSBO_ZOOMED = 1;
        static constexpr uint32_t TARGET_I2MAP_SSBO_ZOOMED_ITERATION = 0;
        static constexpr uint32_t SET_VIDEO = 1;
        static constexpr uint32_t SET_PALETTE = 2;
        static constexpr uint32_t SET_TIME = 3;
        static constexpr uint32_t SET_OUTPUT_IMAGE = 4;
        static constexpr uint32_t BINDING_OUTPUT_MERGED_IMAGE = 0;
        static constexpr uint32_t SET_OUTPUT_ITERATION = 5;
        static constexpr uint32_t SET_STRIPE = 6;
        static constexpr uint32_t SET_TEXTURE = 7;

        // Path currently resident in each layer's sampler; guards against reloading on every shader edit.
        std::array<std::string, TEXTURE_LAYER_COUNT> loadedTexturePaths = {};

        // The merged image this writes is 8-bit for an SDR export and half float for an HDR one, and a
        // storage image's format is fixed in the compiled shader, so the two are separate binaries.
        explicit CPC2MapIterationStripe(vkh::EngineRef engine, const uint32_t windowContextIndex,
                                        const bool hdrChain)
            : ComputePipelineConfigurator(engine, windowContextIndex,
                                          hdrChain
                                              ? "vk_2_map_iter_stripe_hdr.comp"
                                              : "vk_2_map_iter_stripe.comp") {
        }

        ~CPC2MapIterationStripe() override = default;

        CPC2MapIterationStripe(const CPC2MapIterationStripe &) = delete;

        CPC2MapIterationStripe &operator=(const CPC2MapIterationStripe &) = delete;

        CPC2MapIterationStripe(CPC2MapIterationStripe &&) = delete;

        CPC2MapIterationStripe &operator=(CPC2MapIterationStripe &&) = delete;

        void updateQueue(vkh::DescriptorUpdateQueue &queue, uint32_t frameIndex) override;

        [[nodiscard]] const vkh::ImageContext &getOutputColorImage() const {
            return getDescriptor(SET_OUTPUT_IMAGE).get<vkh::StorageImage>(0, BINDING_OUTPUT_MERGED_IMAGE).ctx[0];
        }

        void pipelineInitialized() override;

        void renderContextRefreshed() override;

        void setCurrentFrame(float currentFrame, uint32_t frameIndex) const;

        void setPalette(const ShdPaletteAttribute &palette);

        void setPaletteDynamic(const ShdPaletteAttribute &palette);

        void setStripe(const ShdStripeAttribute &stripe);

        // warpSourceLayer keeps that layer's image loaded even when the layer paints nothing, and
        // residentMask keeps a bit's layer loaded for whatever may ask for it later: nothing is read
        // off disk once an export is running, so a layer a timeline can reach has to be there first.
        void setTextures(const std::array<ShdTextureAttribute, TEXTURE_LAYER_COUNT> &textures, int warpSourceLayer,
                         uint32_t residentMask = 0u);

        void setTextureParams(const std::array<ShdTextureAttribute, TEXTURE_LAYER_COUNT> &textures);

        void setPattern(const std::array<ShdPatternAttribute, PATTERN_LAYER_COUNT> &patterns);

        // Reads the paths setTextures loaded, so call it after that: a warp pointed at a layer
        // holding no image is switched off rather than left reading the placeholder.
        void setWarp(const ShdWarpAttribute &warp);

        void setDefaultZoomIncrement(float defaultZoomIncrement) const;

        void setSampleJitter(float jitterX, float jitterY) const;

        void setDither(bool use) const;

        void setAllIterations(const std::vector<double> &normal, const std::vector<double> &zoomed) const;

        void set2MapSize(const VkExtent2D &extent);

        void setInfo(double maxIteration, double normalMaxIteration, double zoomedMaxIteration) const;

        void setTime(float currentSec, uint32_t frameIndex);

        // Brings every animation phase up to this instant under the speeds in effect until now.
        // Call it before the timeline replaces any of them, or the new speed is charged for time
        // it was not running. A backwards or long jump is a seek, and re-derives them instead.
        void advanceAnimationTo(float sec);

    private:
        // Every animation phase this pass draws with. The shaders are handed the phase rather than
        // the speed, so a timeline track on a speed no longer jumps what it drives.
        ShaderAnimationPhases phases = {};

    protected:
        void configurePushConstant(vkh::PipelineLayoutManagerRef pipelineLayoutManager) override;

        void configureDescriptors(std::vector<vkh::DescriptorPtr> &descriptors) override;
    };
}
