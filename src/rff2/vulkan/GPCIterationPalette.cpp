//
// Created by Merutilm on 2025-07-29.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-07-09, 2026-08-21.
// Modified by Opus 5 on 2026-08-05, 2026-08-06, 2026-08-07, 2026-08-10, 2026-08-13, 2026-08-15, 2026-08-17, 2026-08-18, 2026-08-20, 2026-08-22, 2026-08-26, 2026-08-31
// Modified by ox-alpha on 2026-08-22.
//

#include "../vulkan/GPCIterationPalette.hpp"

#include "PaletteBandLine.hpp"
#include "SharedDescriptorTemplate.hpp"
#include "TextureDescriptor.hpp"
#include "../../vulkan_helper/repo/GlobalSamplerRepo.hpp"
#include "../../vulkan_helper/util/BufferImageContextUtils.hpp"
#include "../../vulkan_helper/util/DescriptorUpdater.hpp"
#include "../attr/ShdPaletteAttribute.h"
#include "../ui/Utilities.h"

namespace merutilm::rff2 {
    float GPCIterationPalette::animationNow() const {
        return animationTimePinned ? pinnedTime : Utilities::getCurrentTime();
    }

    void GPCIterationPalette::updateQueue(vkh::DescriptorUpdateQueue &queue,
                                          const uint32_t frameIndex) {
        using namespace SharedDescriptorTemplate;
        auto &timeDesc = getDescriptor(SET_TIME);
        const auto &timeBinding = *timeDesc.get<vkh::Uniform>(0, DescTime::BINDING_UBO_TIME);

        // The descriptor is shared with the stripe pass, so what is published here is what every
        // pass in this window draws its animation from.
        const float now = animationNow();
        phases.advanceTo(now);
        phases.writeTimeUniform(timeBinding, frameIndex, now);
    }

    void GPCIterationPalette::cmdRefreshIterations(const VkCommandBuffer cbh, const vkh::BufferContext &src) const {
        vkh::BufferImageContextUtils::cmdCopyBuffer(cbh, src, getResultIterationBuffer());
    }

    const vkh::BufferContext &GPCIterationPalette::getResultIterationBuffer() const {
        using namespace SharedDescriptorTemplate;
        auto &iterDesc = getDescriptor(SET_ITERATION);
        const auto &iterSSBO = *iterDesc.get<vkh::ShaderStorage>(0,
                                                                 DescIteration::BINDING_SSBO_ITERATION_MATRIX);
        return iterSSBO.getBufferContext();
    }


    void GPCIterationPalette::resetIterationBuffer(const uint32_t width, const uint32_t height) {
        using namespace SharedDescriptorTemplate;
        auto &iterDesc = getDescriptor(SET_ITERATION);
        const auto &iterUBO = *iterDesc.get<vkh::Uniform>(0, DescIteration::BINDING_UBO_ITERATION_INFO);
        auto &iterUBOHost = iterUBO.getHostObject();
        auto &iterSSBO = *iterDesc.get<vkh::ShaderStorage>(0, DescIteration::BINDING_SSBO_ITERATION_MATRIX);
        auto &iterSSBOHost = iterSSBO.getHostObject();

        this->iterWidth = width;
        this->iterHeight = height;
        iterUBOHost.set<glm::uvec2>(DescIteration::TARGET_UBO_ITERATION_EXTENT, {width, height});
        // The buffer is the whole canvas unless a tiled export says otherwise, so reset it to that here.
        iterUBOHost.set<glm::uvec2>(DescIteration::TARGET_UBO_ITERATION_CANVAS_EXTENT, {width, height});
        iterUBOHost.set<glm::ivec2>(DescIteration::TARGET_UBO_ITERATION_CANVAS_OFFSET, {0, 0});
        iterUBO.update(DescIteration::TARGET_UBO_ITERATION_EXTENT);
        iterUBO.update(DescIteration::TARGET_UBO_ITERATION_CANVAS_EXTENT);
        iterUBO.update(DescIteration::TARGET_UBO_ITERATION_CANVAS_OFFSET);

        iterSSBOHost.resizeArray<double>(DescIteration::TARGET_SSBO_ITERATION_BUFFER, width * height);
        iterSSBO.reloadBuffer();
        iterSSBO.lock(wc.getCommandPool());
        writeDescriptorMF(
            [&iterDesc](vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
                iterDesc.queue(queue, frameIndex, {}, {DescIteration::BINDING_SSBO_ITERATION_MATRIX});
            });
    }

    void GPCIterationPalette::setMaxIteration(const double maxIteration) const {
        using namespace SharedDescriptorTemplate;
        const auto &iterUBO = *getDescriptor(SET_ITERATION).get<vkh::Uniform>(0,
                                                                              DescIteration::BINDING_UBO_ITERATION_INFO);

        auto &iterUBOHost = iterUBO.getHostObject();
        iterUBOHost.set<double>(DescIteration::TARGET_UBO_ITERATION_MAX, maxIteration);
        iterUBOHost.set<double>(DescIteration::TARGET_UBO_ITERATION_MAX_NORMAL, maxIteration);
        iterUBOHost.set<double>(DescIteration::TARGET_UBO_ITERATION_MAX_ZOOMED, maxIteration);
        iterUBO.update();
    }

    void GPCIterationPalette::pinAnimationTime(const bool pin) {
        // Captured on the way in, so the pinned frame matches what the preview was showing.
        pinnedTime = Utilities::getCurrentTime();
        animationTimePinned = pin;
    }

    void GPCIterationPalette::setCanvasGeometry(const glm::uvec2 &canvasExtent,
                                                const glm::ivec2 &canvasOffset) const {
        using namespace SharedDescriptorTemplate;
        const auto &iterUBO = *getDescriptor(SET_ITERATION).get<vkh::Uniform>(
            0, DescIteration::BINDING_UBO_ITERATION_INFO);
        auto &iterUBOHost = iterUBO.getHostObject();
        iterUBOHost.set<glm::uvec2>(DescIteration::TARGET_UBO_ITERATION_CANVAS_EXTENT, canvasExtent);
        iterUBOHost.set<glm::ivec2>(DescIteration::TARGET_UBO_ITERATION_CANVAS_OFFSET, canvasOffset);
        iterUBO.update(DescIteration::TARGET_UBO_ITERATION_CANVAS_EXTENT);
        iterUBO.update(DescIteration::TARGET_UBO_ITERATION_CANVAS_OFFSET);
    }

    void GPCIterationPalette::setPalette(const ShdPaletteAttribute &palette) {
        using namespace SharedDescriptorTemplate;
        // Freeze the phase reached under the old speeds before adopting the new ones, so a speed
        // edit continues from where the animation stands instead of restarting somewhere else.
        phases.advanceTo(animationNow());
        phases.setPaletteSpeeds(palette);

        auto &paletteDesc = getDescriptor(SET_PALETTE);
        auto &paletteSSBO = *paletteDesc.get<vkh::ShaderStorage>(0, DescPalette::BINDING_SSBO_PALETTE);
        auto &paletteSSBOHost = paletteSSBO.getHostObject();

        if (paletteSSBO.isLocked()) {
            paletteSSBO.unlock(wc.getCommandPool());
        }

        std::vector<glm::vec4> baseColors;
        if (palette.seamless) {
            baseColors = palette.colors;
            baseColors.insert(baseColors.end(), palette.colors.rbegin(), palette.colors.rend());
        } else {
            baseColors = palette.colors;
        }

        std::vector<glm::vec4> finalColors;
        if (palette.enableGloss) {
            finalColors.reserve(baseColors.size());
            for (const auto& c : baseColors) {
                const float t = static_cast<float>(&c - &baseColors[0]) / static_cast<float>(baseColors.size());
                const float highlight = std::pow(0.5f + 0.5f * std::sin(t * 3.14159f * 6.0f), 20.0f);
                float r = std::pow(c.r, 1.2f);
                float g = std::pow(c.g, 1.2f);
                float b = std::pow(c.b, 1.2f);
                finalColors.emplace_back(std::min(1.0f, r + highlight * 0.8f * palette.glossColor.r), std::min(1.0f, g + highlight * 0.8f * palette.glossColor.g), std::min(1.0f, b + highlight * 0.8f * palette.glossColor.b), c.a);
            }
        } else {
            finalColors = baseColors;
        }

        finalColors = applyBandLines(std::move(finalColors), palette);

        const auto finalSize = static_cast<uint32_t>(finalColors.size());

        paletteSSBOHost.set<uint32_t>(DescPalette::TARGET_PALETTE_SIZE, finalSize);

        paletteSSBOHost.set<glm::vec4>(DescPalette::TARGET_PALETTE_INTERVAL, palette.iterationInterval);
        paletteSSBOHost.set<double>(DescPalette::TARGET_PALETTE_OFFSET, palette.offsetRatio);
        // The fixed part is packed solid up to the color array, so the blend's mode rides in the upper bits: low 8 = smoothing, bit 8 = space.
        // Bit 9 carries the cycle curve choice, and bits 10-13 the iteration coloring mode.
        paletteSSBOHost.set<uint32_t>(DescPalette::TARGET_PALETTE_SMOOTHING,
                                      (static_cast<uint32_t>(palette.colorSmoothing) & 0xFFu) |
                                      (static_cast<uint32_t>(palette.colorInterpolation) << 8) |
                                      (static_cast<uint32_t>(palette.cycleCurve) << 9) |
                                      ((static_cast<uint32_t>(palette.iterationColoring) & 0xFu) << 10));
        paletteSSBOHost.set<float>(DescPalette::TARGET_PALETTE_ANIMATION_SPEED, palette.animationSpeed);
        paletteSSBOHost.set<uint32_t>(DescPalette::TARGET_PALETTE_ANIMATION_MODE, static_cast<uint32_t>(palette.animationMode));
        paletteSSBOHost.set<float>(DescPalette::TARGET_PALETTE_ANIMATION_FLOW_AMOUNT, palette.animationFlowAmount);
        paletteSSBOHost.set<float>(DescPalette::TARGET_PALETTE_ANIMATION_FLOW_SCALE, palette.animationFlowScale);
        paletteSSBOHost.set<float>(DescPalette::TARGET_PALETTE_ANIMATION_FLOW_SPEED, palette.animationFlowSpeed);
        paletteSSBOHost.set<float>(DescPalette::TARGET_PALETTE_ANIMATION_FLOW_SWIRL, palette.animationFlowSwirl);

        uint32_t staticCount = static_cast<uint32_t>(palette.staticColorIterations.size());
        if (staticCount > DescPalette::MAX_STATIC_COLORS) staticCount = DescPalette::MAX_STATIC_COLORS;
        paletteSSBOHost.set<uint32_t>(DescPalette::TARGET_PALETTE_STATIC_COLOR_COUNT, staticCount);
        paletteSSBOHost.set<float>(DescPalette::TARGET_PALETTE_STATIC_COLOR_TOLERANCE, palette.staticColorTolerance);
        paletteSSBOHost.reset(DescPalette::TARGET_PALETTE_STATIC_COLOR_ITERATIONS);
        for (uint32_t i = 0; i < staticCount; ++i) {
            double v = palette.staticColorIterations[i];
            paletteSSBOHost.set<double>(DescPalette::TARGET_PALETTE_STATIC_COLOR_ITERATIONS, i, v);
        }

        paletteSSBOHost.set<glm::vec4>(DescPalette::TARGET_PALETTE_MANDELBROT_COLOR, palette.mandelbrotColor);
        paletteSSBOHost.set<float>(DescPalette::TARGET_PALETTE_CYCLE_BIAS, palette.cycleBias);
        paletteSSBOHost.resizeArray<glm::vec4>(DescPalette::TARGET_PALETTE_COLORS, finalColors.size());
        paletteSSBOHost.set<glm::vec4>(DescPalette::TARGET_PALETTE_COLORS, finalColors);

        paletteSSBO.reloadBuffer();
        paletteSSBO.update();
        paletteSSBO.lock(wc.getCommandPool());

        writeDescriptorMF(
            [&paletteDesc](vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
                paletteDesc.queue(queue, frameIndex, {}, {DescPalette::BINDING_SSBO_PALETTE});
            });
    }

    void GPCIterationPalette::setTextures(const std::array<ShdTextureAttribute, TEXTURE_LAYER_COUNT> &textures,
                                          const int warpSourceLayer) {
        phases.advanceTo(animationNow());
        auto &textureDesc = getDescriptor(SET_TEXTURE);
        std::vector<uint32_t> changedBindings;
        for (uint32_t layer = 0; layer < TEXTURE_LAYER_COUNT; ++layer) {
            const auto &texture = textures[layer];
            phases.setTextureSpeed(layer, texture);
            const bool wanted = texture.enabled || static_cast<int>(layer) == warpSourceLayer;
            if (TextureDescriptor::uploadImage(wc.core, wc.getCommandPool(), textureDesc, layer,
                                               wanted ? texture.path : std::string{},
                                               loadedTexturePaths[layer])) {
                changedBindings.push_back(TextureDescriptor::samplerBinding(layer));
            }
            TextureDescriptor::updateParams(textureDesc, layer, texture, !loadedTexturePaths[layer].empty());
        }
        if (changedBindings.empty()) {
            return;
        }
        writeDescriptorMF(
            [&textureDesc, changedBindings = std::move(changedBindings)](vkh::DescriptorUpdateQueue &queue,
                                                                        const uint32_t frameIndex) {
                textureDesc.queue(queue, frameIndex, {}, std::vector(changedBindings));
            });
    }

    void GPCIterationPalette::setPattern(const std::array<ShdPatternAttribute, PATTERN_LAYER_COUNT> &patterns) {
        phases.advanceTo(animationNow());
        // Shares the texture set's UBO, so no descriptor write is needed beyond the buffer update.
        for (uint32_t layer = 0; layer < PATTERN_LAYER_COUNT; ++layer) {
            phases.setPatternSpeed(layer, patterns[layer]);
            TextureDescriptor::updatePatternParams(getDescriptor(SET_TEXTURE), layer, patterns[layer]);
        }
    }

    void GPCIterationPalette::setWarp(const ShdWarpAttribute &warp) {
        phases.advanceTo(animationNow());
        phases.setWarpSpeed(warp);
        const int sourceLayer = warpSourceLayer(warp);
        const bool sourceReady = sourceLayer < 0 || !loadedTexturePaths[sourceLayer].empty();
        TextureDescriptor::updateWarpParams(getDescriptor(SET_TEXTURE), warp, sourceReady);
    }

    void GPCIterationPalette::setStripeSpeed(const ShdStripeAttribute &stripe) {
        phases.advanceTo(animationNow());
        phases.setStripeSpeed(stripe);
    }

    void GPCIterationPalette::pipelineInitialized() {
        using namespace SharedDescriptorTemplate;
        const auto &timeDesc = getDescriptor(SET_TIME);
        const auto &iterDesc = getDescriptor(SET_ITERATION);
        auto &textureDesc = getDescriptor(SET_TEXTURE);
        // Every sampler must reference a real image before the first draw, even with no texture chosen.
        std::vector<uint32_t> textureBindings;
        for (uint32_t layer = 0; layer < TEXTURE_LAYER_COUNT; ++layer) {
            TextureDescriptor::uploadPlaceholder(wc.core, wc.getCommandPool(), textureDesc, layer);
            TextureDescriptor::updateParams(textureDesc, layer, ShdTextureAttribute{}, false);
            textureBindings.push_back(TextureDescriptor::samplerBinding(layer));
        }
        textureBindings.push_back(TextureDescriptor::BINDING_UBO_TEXTURE);
        for (uint32_t layer = 0; layer < PATTERN_LAYER_COUNT; ++layer) {
            TextureDescriptor::updatePatternParams(textureDesc, layer, ShdPatternAttribute{});
        }
        TextureDescriptor::updateWarpParams(textureDesc, ShdWarpAttribute{}, true);
        writeDescriptorMF(
            [&timeDesc, &iterDesc, &textureDesc, textureBindings = std::move(textureBindings)](
        vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
                timeDesc.queue(queue, frameIndex, {}, {DescTime::BINDING_UBO_TIME});
                iterDesc.queue(queue, frameIndex, {}, {DescIteration::BINDING_UBO_ITERATION_INFO});
                textureDesc.queue(queue, frameIndex, {}, std::vector(textureBindings));
            });
    }

    void GPCIterationPalette::renderContextRefreshed() {
        //no operation
    }


    void GPCIterationPalette::configurePushConstant(
        vkh::PipelineLayoutManagerRef pipelineLayoutManager) {
        //noop
    }

    void GPCIterationPalette::configureDescriptors(std::vector<vkh::DescriptorPtr> &descriptors) {
        using namespace SharedDescriptorTemplate;
        appendDescriptor<DescIteration>(SET_ITERATION, descriptors);
        appendDescriptor<DescPalette>(SET_PALETTE, descriptors);
        appendDescriptor<DescTime>(SET_TIME, descriptors);

        // REPEAT so the texture tiles when the scale or scroll pushes the coordinates past 0..1.
        const vkh::SamplerRef sampler = pickFromGlobalRepository<vkh::GlobalSamplerRepo, vkh::SamplerRef>(
            VkSamplerCreateInfo{
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .magFilter = VK_FILTER_LINEAR,
                .minFilter = VK_FILTER_LINEAR,
                .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
                .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .mipLodBias = 0,
                .anisotropyEnable = VK_FALSE,
                .maxAnisotropy = 0,
                .compareEnable = VK_FALSE,
                .compareOp = VK_COMPARE_OP_ALWAYS,
                .minLod = 0,
                .maxLod = 0,
                .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
                .unnormalizedCoordinates = VK_FALSE,
            });
        appendUniqueDescriptor(SET_TEXTURE, descriptors,
                               TextureDescriptor::createManager(wc.core, sampler, VK_SHADER_STAGE_FRAGMENT_BIT));
    }
}
