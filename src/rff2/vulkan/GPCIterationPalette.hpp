//
// Created by Merutilm on 2025-07-29.
// Modified by Opus 5 on 2026-08-05, 2026-08-06, 2026-08-07, 2026-08-10, 2026-08-13, 2026-08-15, 2026-08-17, 2026-08-18, 2026-08-26
// Modified by Fable 5.1 on 2026-09-06
// Modified by GPT-6 on 2026-09-11, 2026-09-14, 2026-09-16, 2026-09-18, 2026-09-21, 2026-09-23
// Modified by Opus 5.5 on 2026-10-07, 2026-10-08, 2026-10-12, 2026-10-16, 2026-10-24
//

#pragma once
#include "ShaderLayerControl.hpp"
#include <array>
#include <string>

#include "ShaderAnimationPhases.hpp"
#include "PreviewAnimationClock.hpp"
#include "ShaderModeSpecialization.hpp"
#include "../../vulkan_helper/configurator/GeneralPostProcessGraphicsPipelineConfigurator.hpp"
#include "../attr/ShdPaletteAttribute.h"
#include "../attr/ShdStripeAttribute.h"
#include "../attr/ShdPatternAttribute.h"
#include "../attr/ShdSlopeAttribute.h"
#include "../attr/ShdTextureAttribute.h"
#include "../attr/ShdWarpAttribute.h"

namespace merutilm::rff2 {
    struct GPCIterationPalette final : public vkh::GeneralPostProcessGraphicsPipelineConfigurator {
        vkh::PushConstant layerPush;
        static constexpr uint32_t SET_ITERATION = 0;
        static constexpr uint32_t SET_PALETTE = 1;
        static constexpr uint32_t SET_TIME = 2;
        static constexpr uint32_t SET_TEXTURE = 3;

        // Path currently resident in each layer's sampler; guards against reloading on every shader edit.
        std::array<std::string, TEXTURE_LAYER_COUNT> loadedTexturePaths = {};
        uint32_t pendingTextureBindings = 0;
        // Every animation phase this pass draws with, carried across speed changes. Each setter
        // brings them up to the moment before adopting the speeds it was given, so dragging a speed
        // slider no longer jumps the animation by (elapsed * speed delta). The stripe's phase is
        // held here too: the time uniform is shared with the stripe pass, and this is what writes it.
        ShaderAnimationPhases phases = {};
        PreviewAnimationClock previewClock;
        float submittedAnimationTime = 0.0f;
        // While set, updateQueue holds this instant instead of reading the clock. A tiled export
        // submits one frame per tile, so without it every tile lands on a different animation phase.
        bool animationTimePinned = false;
        float pinnedTime = 0.0f;
        // The modes the pipeline is specialized on; every setter that moves one rebuilds the pipeline.
        ShaderModeSpecialization specModes = {};

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

        // Copies both staging buffers, the iterations and the escape angles, onto the GPU buffers the passes read.
        void cmdRefreshIterations(VkCommandBuffer cbh, const vkh::BufferContext &iterationSrc,
                                  const vkh::BufferContext &escapeAngleSrc,
                                  const vkh::BufferContext &orbitTrapSrc) const;

        const vkh::BufferContext &getResultIterationBuffer() const;

        const vkh::BufferContext &getResultEscapeAngleBuffer() const;

        const vkh::BufferContext &getResultOrbitTrapBuffer() const;

        // trapPlanes is the plane count of the orbit trap staging buffer copied in each frame.
        void resetIterationBuffer(uint32_t width, uint32_t height, uint32_t trapPlanes);

        // Resizes only the orbit trap buffer, for a staging buffer that changed its plane count.
        void resizeOrbitTrapBuffer(uint32_t width, uint32_t height, uint32_t trapPlanes);

        void setMaxIteration(double maxIteration) const;

        // Freezes (or releases) the animation clock so a multi-frame job renders one instant.
        void pinAnimationTime(bool pin);
        void setPreviewAnimationPaused(bool paused);
        void swapAnimationLayers(AnimatedLayerFamily family, uint32_t from, uint32_t to);

        // Geometry of the whole canvas this buffer is a piece of, for the screen-space animation
        // fields and decor UVs. A normal frame is the whole canvas, so extent with a zero offset.
        void setCanvasGeometry(const glm::uvec2 &canvasExtent, const glm::ivec2 &canvasOffset) const;

        // The slope supplies only the light the orbit trap relief is lit from.
        void setPalette(const ShdPaletteAttribute &palette, const ShdSlopeAttribute &slope);

        // The stripe pass owns its own uniform but not the clock: its phase belongs to the time
        // uniform this pass publishes, so the speed it runs at is handed over here as well.
        void setStripeSpeed(const ShdStripeAttribute &stripe);

        // warpSourceLayer keeps that layer's image loaded even when the layer paints nothing.
        // residentMask keeps a bit's layer loaded although the layer paints nothing, as the Studio environment's source does.
        void setTextures(const std::array<ShdTextureAttribute, TEXTURE_LAYER_COUNT> &textures, int warpSourceLayer,
                         uint32_t residentMask = 0u);

        void setPattern(const std::array<ShdPatternAttribute, PATTERN_LAYER_COUNT> &patterns);

        // Reads the paths setTextures loaded, so call it after that: a warp pointed at a layer
        // holding no image is switched off rather than left reading the placeholder.
        void setEffects(const ShdEffectsAttribute &effects);

        void setWarp(const ShdWarpAttribute &warp);

        void pipelineInitialized() override;

        void renderContextRefreshed() override;

        [[nodiscard]] std::vector<uint32_t> specializationConstants() const override;

    private:
        // The instant the animation stands at: the clock, unless a multi-frame job pinned it.
        [[nodiscard]] float animationNow() const;

    protected:
        void configurePushConstant(vkh::PipelineLayoutManagerRef pipelineLayoutManager) override;

        void configureDescriptors(std::vector<vkh::DescriptorPtr> &descriptors) override;

    };
}
