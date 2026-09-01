//
// Created by Merutilm on 2025-07-29.
// Modified by Opus 5 on 2026-08-05, 2026-08-06, 2026-08-07, 2026-08-10, 2026-08-13, 2026-08-15, 2026-08-17, 2026-08-18, 2026-08-26
//

#pragma once
#include <array>
#include <string>

#include "ShaderAnimationPhases.hpp"
#include "../../vulkan_helper/configurator/GeneralPostProcessGraphicsPipelineConfigurator.hpp"
#include "../attr/ShdPaletteAttribute.h"
#include "../attr/ShdStripeAttribute.h"
#include "../attr/ShdPatternAttribute.h"
#include "../attr/ShdTextureAttribute.h"
#include "../attr/ShdWarpAttribute.h"

namespace merutilm::rff2 {
    struct GPCIterationPalette final : public vkh::GeneralPostProcessGraphicsPipelineConfigurator {
        static constexpr uint32_t SET_ITERATION = 0;
        static constexpr uint32_t SET_PALETTE = 1;
        static constexpr uint32_t SET_TIME = 2;
        static constexpr uint32_t SET_TEXTURE = 3;

        uint32_t iterWidth = 0;
        uint32_t iterHeight = 0;
        // Path currently resident in each layer's sampler; guards against reloading on every shader edit.
        std::array<std::string, TEXTURE_LAYER_COUNT> loadedTexturePaths = {};
        // Every animation phase this pass draws with, carried across speed changes. Each setter
        // brings them up to the moment before adopting the speeds it was given, so dragging a speed
        // slider no longer jumps the animation by (elapsed * speed delta). The stripe's phase is
        // held here too: the time uniform is shared with the stripe pass, and this is what writes it.
        ShaderAnimationPhases phases = {};
        // While set, updateQueue holds this instant instead of reading the clock. A tiled export
        // submits one frame per tile, so without it every tile lands on a different animation phase.
        bool animationTimePinned = false;
        float pinnedTime = 0.0f;

        GPCIterationPalette(vkh::EngineRef engine, const uint32_t windowContextIndex,
                                             const uint32_t renderContextIndex,
                                             const uint32_t primarySubpassIndex) : GeneralPostProcessGraphicsPipelineConfigurator(
            engine, windowContextIndex, renderContextIndex, primarySubpassIndex,
            "vk_iteration_palette.frag") {
        };

        ~GPCIterationPalette() override = default;

        GPCIterationPalette(const GPCIterationPalette &) = delete;

        GPCIterationPalette &operator=(const GPCIterationPalette &) = delete;

        GPCIterationPalette(GPCIterationPalette &&) = delete;

        GPCIterationPalette &operator=(GPCIterationPalette &&) = delete;

        void updateQueue(vkh::DescriptorUpdateQueue &queue, uint32_t frameIndex) override;

        void cmdRefreshIterations(VkCommandBuffer cbh, const vkh::BufferContext &src) const;

        const vkh::BufferContext &getResultIterationBuffer() const;

        void resetIterationBuffer(uint32_t width, uint32_t height);

        void setMaxIteration(double maxIteration) const;

        // Freezes (or releases) the animation clock so a multi-frame job renders one instant.
        void pinAnimationTime(bool pin);

        // Geometry of the whole canvas this buffer is a piece of, for the screen-space animation
        // fields and decor UVs. A normal frame is the whole canvas, so extent with a zero offset.
        void setCanvasGeometry(const glm::uvec2 &canvasExtent, const glm::ivec2 &canvasOffset) const;

        void setPalette(const ShdPaletteAttribute &palette);

        // The stripe pass owns its own uniform but not the clock: its phase belongs to the time
        // uniform this pass publishes, so the speed it runs at is handed over here as well.
        void setStripeSpeed(const ShdStripeAttribute &stripe);

        // warpSourceLayer keeps that layer's image loaded even when the layer paints nothing.
        void setTextures(const std::array<ShdTextureAttribute, TEXTURE_LAYER_COUNT> &textures, int warpSourceLayer);

        void setPattern(const std::array<ShdPatternAttribute, PATTERN_LAYER_COUNT> &patterns);

        // Reads the paths setTextures loaded, so call it after that: a warp pointed at a layer
        // holding no image is switched off rather than left reading the placeholder.
        void setWarp(const ShdWarpAttribute &warp);

        void pipelineInitialized() override;

        void renderContextRefreshed() override;

    private:
        // The instant the animation stands at: the clock, unless a multi-frame job pinned it.
        [[nodiscard]] float animationNow() const;

    protected:
        void configurePushConstant(vkh::PipelineLayoutManagerRef pipelineLayoutManager) override;

        void configureDescriptors(std::vector<vkh::DescriptorPtr> &descriptors) override;

    };
}
