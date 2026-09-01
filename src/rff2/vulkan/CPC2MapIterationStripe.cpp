//
// Created by Merutilm on 2025-09-06.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-07-09, 2026-08-21.
// Modified by Opus 5 on 2026-08-05, 2026-08-07, 2026-08-10, 2026-08-13, 2026-08-15, 2026-08-17, 2026-08-18, 2026-08-20, 2026-08-22, 2026-08-24, 2026-08-25, 2026-08-26, 2026-08-31
// Modified by ox-alpha on 2026-08-22.
//

#include "CPC2MapIterationStripe.hpp"

#include "PaletteBandLine.hpp"
#include "SharedDescriptorTemplate.hpp"
#include "SharedImageContextIndices.hpp"
#include "TextureDescriptor.hpp"
#include "../../vulkan_helper/repo/GlobalSamplerRepo.hpp"
#include "../attr/ShdPaletteAttribute.h"

namespace merutilm::rff2 {
    void CPC2MapIterationStripe::updateQueue(vkh::DescriptorUpdateQueue &queue, uint32_t frameIndex) {
        //noop
    }


    void CPC2MapIterationStripe::pipelineInitialized() {
        using namespace SharedDescriptorTemplate;
        const auto &iterDesc = getDescriptor(SET_OUTPUT_ITERATION);
        const auto &stripeDesc = getDescriptor(SET_STRIPE);
        const auto &timeDesc = getDescriptor(SET_TIME);
        const auto &vidDesc = getDescriptor(SET_VIDEO);
        auto &textureDesc = getDescriptor(SET_TEXTURE);
        // Every sampler must reference a real image before the first dispatch, even with no texture chosen.
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
        writeDescriptorMF([&iterDesc, &stripeDesc, &timeDesc, &vidDesc, &textureDesc,
                textureBindings = std::move(textureBindings)](vkh::DescriptorUpdateQueue &queue,
                                                              const uint32_t frameIndex) {
            iterDesc.queue(queue, frameIndex, {}, {DescIteration::BINDING_UBO_ITERATION_INFO});
            stripeDesc.queue(queue, frameIndex, {}, {DescStripe::BINDING_UBO_STRIPE});
            timeDesc.queue(queue, frameIndex, {}, {DescTime::BINDING_UBO_TIME});
            vidDesc.queue(queue, frameIndex, {}, {DescVideo::BINDING_UBO_VIDEO});
            textureDesc.queue(queue, frameIndex, {}, std::vector(textureBindings));
        });
    }

    void CPC2MapIterationStripe::renderContextRefreshed() {
        using namespace SharedImageContextIndices;
        auto &outDesc = getDescriptor(SET_OUTPUT_IMAGE);
        auto &[outImg] = outDesc.get<vkh::StorageImage>(0, BINDING_OUTPUT_MERGED_IMAGE);
        outImg = wc.getSharedImageContext().getImageContextMF(MF_VIDEO_RENDER_IMAGE_PRIMARY);
        writeDescriptorMF(
            [&outDesc](vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
                outDesc.queue(queue, frameIndex, {}, {BINDING_OUTPUT_MERGED_IMAGE});
            });
    }


    void CPC2MapIterationStripe::setCurrentFrame(const float currentFrame, const uint32_t frameIndex) const {
        using namespace SharedDescriptorTemplate;
        auto &vidDesc = getDescriptor(SET_VIDEO);
        const auto &vidUBO = *vidDesc.get<vkh::Uniform>(0, DescVideo::BINDING_UBO_VIDEO);
        auto &vidUBOHost = vidUBO.getHostObject();
        vidUBOHost.set<float>(DescVideo::TARGET_VIDEO_CURRENT_FRAME, currentFrame);
        vidUBO.updateMF(frameIndex);
    }


    void CPC2MapIterationStripe::setPalette(const ShdPaletteAttribute &palette) {
        using namespace SharedDescriptorTemplate;
        phases.setPaletteSpeeds(palette);
        auto &paletteDesc = getDescriptor(SET_PALETTE);
        auto &paletteSSBO = *paletteDesc.get<vkh::ShaderStorage>(0,
                                                                 DescPalette::BINDING_SSBO_PALETTE);
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
        // Same packing as GPCIterationPalette: low 8 bits = smoothing method, bit 8 = interpolation space.
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

        writeDescriptorMF(
            [&paletteDesc](vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
                paletteDesc.queue(queue, frameIndex, {}, {DescPalette::BINDING_SSBO_PALETTE});
            });
    }

    void CPC2MapIterationStripe::setPaletteDynamic(const ShdPaletteAttribute &palette) {
        using namespace SharedDescriptorTemplate;
        phases.setPaletteSpeeds(palette);
        auto &paletteDesc = getDescriptor(SET_PALETTE);
        const auto &paletteSSBO = *paletteDesc.get<vkh::ShaderStorage>(0, DescPalette::BINDING_SSBO_PALETTE);
        auto &host = paletteSSBO.getHostObject();
        host.set<glm::vec4>(DescPalette::TARGET_PALETTE_INTERVAL, palette.iterationInterval);
        host.set<double>(DescPalette::TARGET_PALETTE_OFFSET, palette.offsetRatio);
        host.set<float>(DescPalette::TARGET_PALETTE_ANIMATION_SPEED, palette.animationSpeed);
        host.set<float>(DescPalette::TARGET_PALETTE_ANIMATION_FLOW_AMOUNT, palette.animationFlowAmount);
        host.set<float>(DescPalette::TARGET_PALETTE_ANIMATION_FLOW_SCALE, palette.animationFlowScale);
        host.set<float>(DescPalette::TARGET_PALETTE_ANIMATION_FLOW_SPEED, palette.animationFlowSpeed);
        host.set<float>(DescPalette::TARGET_PALETTE_ANIMATION_FLOW_SWIRL, palette.animationFlowSwirl);
        host.set<float>(DescPalette::TARGET_PALETTE_STATIC_COLOR_TOLERANCE, palette.staticColorTolerance);
        host.set<glm::vec4>(DescPalette::TARGET_PALETTE_MANDELBROT_COLOR, palette.mandelbrotColor);
        host.set<float>(DescPalette::TARGET_PALETTE_CYCLE_BIAS, palette.cycleBias);
        // The cycle curve and the iteration coloring ride in the smoothing word, so the whole word is rewritten to carry them.
        host.set<uint32_t>(DescPalette::TARGET_PALETTE_SMOOTHING,
                           (static_cast<uint32_t>(palette.colorSmoothing) & 0xFFu) |
                           (static_cast<uint32_t>(palette.colorInterpolation) << 8) |
                           (static_cast<uint32_t>(palette.cycleCurve) << 9) |
                           ((static_cast<uint32_t>(palette.iterationColoring) & 0xFu) << 10));
        paletteSSBO.update(DescPalette::TARGET_PALETTE_INTERVAL);
        paletteSSBO.update(DescPalette::TARGET_PALETTE_OFFSET);
        paletteSSBO.update(DescPalette::TARGET_PALETTE_ANIMATION_SPEED);
        paletteSSBO.update(DescPalette::TARGET_PALETTE_ANIMATION_FLOW_AMOUNT);
        paletteSSBO.update(DescPalette::TARGET_PALETTE_ANIMATION_FLOW_SCALE);
        paletteSSBO.update(DescPalette::TARGET_PALETTE_ANIMATION_FLOW_SPEED);
        paletteSSBO.update(DescPalette::TARGET_PALETTE_ANIMATION_FLOW_SWIRL);
        paletteSSBO.update(DescPalette::TARGET_PALETTE_STATIC_COLOR_TOLERANCE);
        paletteSSBO.update(DescPalette::TARGET_PALETTE_MANDELBROT_COLOR);
        paletteSSBO.update(DescPalette::TARGET_PALETTE_CYCLE_BIAS);
        paletteSSBO.update(DescPalette::TARGET_PALETTE_SMOOTHING);
    }

    void CPC2MapIterationStripe::setStripe(const ShdStripeAttribute &stripe) {
        using namespace SharedDescriptorTemplate;
        phases.setStripeSpeed(stripe);
        auto &stripeDesc = getDescriptor(SET_STRIPE);
        const auto &stripeUBO = *stripeDesc.get<vkh::Uniform>(0, DescStripe::BINDING_UBO_STRIPE);
        auto &stripeUBOHost = stripeUBO.getHostObject();
        stripeUBOHost.set(DescStripe::TARGET_STRIPE_TYPE, static_cast<uint32_t>(stripe.stripeType));
        stripeUBOHost.set(DescStripe::TARGET_STRIPE_FIRST_INTERVAL,
                          stripe.firstInterval);
        stripeUBOHost.set(DescStripe::TARGET_STRIPE_SECOND_INTERVAL,
                          stripe.secondInterval);
        stripeUBOHost.set(DescStripe::TARGET_STRIPE_OPACITY, stripe.opacity);
        stripeUBOHost.set(DescStripe::TARGET_STRIPE_OFFSET, stripe.offset);
        stripeUBOHost.set(DescStripe::TARGET_STRIPE_ANIMATION_SPEED,
                          stripe.animationSpeed);
        stripeUBO.update();
    }

    void CPC2MapIterationStripe::setTextures(const std::array<ShdTextureAttribute, TEXTURE_LAYER_COUNT> &textures,
                                              const int warpSourceLayer, const uint32_t residentMask) {
        auto &textureDesc = getDescriptor(SET_TEXTURE);
        std::vector<uint32_t> changedBindings;
        for (uint32_t layer = 0; layer < TEXTURE_LAYER_COUNT; ++layer) {
            const auto &texture = textures[layer];
            const bool wanted = texture.enabled || static_cast<int>(layer) == warpSourceLayer ||
                                (residentMask >> layer & 1u) != 0u;
            if (TextureDescriptor::uploadImage(wc.core, wc.getCommandPool(), textureDesc, layer,
                                               wanted ? texture.path : std::string{},
                                               loadedTexturePaths[layer])) {
                changedBindings.push_back(TextureDescriptor::samplerBinding(layer));
            }
            phases.setTextureSpeed(layer, texture);
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

    void CPC2MapIterationStripe::setTextureParams(
        const std::array<ShdTextureAttribute, TEXTURE_LAYER_COUNT> &textures) {
        auto &textureDesc = getDescriptor(SET_TEXTURE);
        for (uint32_t layer = 0; layer < TEXTURE_LAYER_COUNT; ++layer) {
            phases.setTextureSpeed(layer, textures[layer]);
            TextureDescriptor::updateParams(textureDesc, layer, textures[layer],
                                            !loadedTexturePaths[layer].empty());
        }
    }


    void CPC2MapIterationStripe::setPattern(const std::array<ShdPatternAttribute, PATTERN_LAYER_COUNT> &patterns) {
        // Shares the texture set's UBO, so no descriptor write is needed beyond the buffer update.
        for (uint32_t layer = 0; layer < PATTERN_LAYER_COUNT; ++layer) {
            phases.setPatternSpeed(layer, patterns[layer]);
            TextureDescriptor::updatePatternParams(getDescriptor(SET_TEXTURE), layer, patterns[layer]);
        }
    }

    void CPC2MapIterationStripe::setWarp(const ShdWarpAttribute &warp) {
        phases.setWarpSpeed(warp);
        const int sourceLayer = warpSourceLayer(warp);
        const bool sourceReady = sourceLayer < 0 || !loadedTexturePaths[sourceLayer].empty();
        TextureDescriptor::updateWarpParams(getDescriptor(SET_TEXTURE), warp, sourceReady);
    }


    void CPC2MapIterationStripe::setDefaultZoomIncrement(const float defaultZoomIncrement) const {
        using namespace SharedDescriptorTemplate;
        auto &vidDesc = getDescriptor(SET_VIDEO);
        const auto &vidUBO = *vidDesc.get<vkh::Uniform>(0, DescVideo::BINDING_UBO_VIDEO);
        auto &vidUBOHost = vidUBO.getHostObject();
        vidUBOHost.set<float>(DescVideo::TARGET_VIDEO_DEFAULT_ZOOM_INCREMENT, defaultZoomIncrement);
        updateBufferMF([&vidUBO](const uint32_t frameIndex) {
            vidUBO.updateMF(frameIndex);
        });
    }


    void CPC2MapIterationStripe::setSampleJitter(const float jitterX, const float jitterY) const {
        using namespace SharedDescriptorTemplate;
        auto &vidDesc = getDescriptor(SET_VIDEO);
        const auto &vidUBO = *vidDesc.get<vkh::Uniform>(0, DescVideo::BINDING_UBO_VIDEO);
        auto &vidUBOHost = vidUBO.getHostObject();
        vidUBOHost.set<glm::vec2>(DescVideo::TARGET_VIDEO_SAMPLE_JITTER, glm::vec2(jitterX, jitterY));
        updateBufferMF([&vidUBO](const uint32_t frameIndex) {
            vidUBO.updateMF(frameIndex);
        });
    }

    void CPC2MapIterationStripe::setDither(const bool use) const {
        using namespace SharedDescriptorTemplate;
        auto &vidDesc = getDescriptor(SET_VIDEO);
        const auto &vidUBO = *vidDesc.get<vkh::Uniform>(0, DescVideo::BINDING_UBO_VIDEO);
        auto &vidUBOHost = vidUBO.getHostObject();
        vidUBOHost.set<bool>(DescVideo::TARGET_VIDEO_DITHER, use);
        updateBufferMF([&vidUBO](const uint32_t frameIndex) {
            vidUBO.updateMF(frameIndex);
        });
    }

    void CPC2MapIterationStripe::setAllIterations(const std::vector<double> &normal,
                                                   const std::vector<double> &zoomed) const {
        using namespace SharedDescriptorTemplate;
        auto &map2Desc = getDescriptor(SET_I2MAP);
        const auto &map2DescNormalSSBO = *map2Desc.get<vkh::ShaderStorage>(0, BINDING_I2MAP_SSBO_NORMAL);
        map2DescNormalSSBO.getHostObject().set<double>(
            TARGET_I2MAP_SSBO_NORMAL_ITERATION, normal);
        const auto &map2DescZoomedSSBO = *map2Desc.get<vkh::ShaderStorage>(0, BINDING_I2MAP_SSBO_ZOOMED);
        map2DescZoomedSSBO.getHostObject().set<double>(
            TARGET_I2MAP_SSBO_ZOOMED_ITERATION, zoomed);

        map2DescNormalSSBO.update();
        map2DescZoomedSSBO.update();
    }

    void CPC2MapIterationStripe::set2MapSize(const VkExtent2D &extent) {
        using namespace SharedDescriptorTemplate;
        const auto &[width, height] = extent;
        setExtent(extent);
        auto &iter = getDescriptor(SET_I2MAP);
        auto &iterNormalSSBO = *iter.get<vkh::ShaderStorage>(0, BINDING_I2MAP_SSBO_NORMAL);
        iterNormalSSBO.getHostObject().resizeAndClear<double>(TARGET_I2MAP_SSBO_NORMAL_ITERATION, width * height);
        iterNormalSSBO.reloadBuffer();

        auto &iterZoomedSSBO = *iter.get<vkh::ShaderStorage>(0, BINDING_I2MAP_SSBO_ZOOMED);
        iterZoomedSSBO.getHostObject().resizeAndClear<double>(TARGET_I2MAP_SSBO_ZOOMED_ITERATION, width * height);
        iterZoomedSSBO.reloadBuffer();

        auto &iterOut = getDescriptor(SET_OUTPUT_ITERATION);
        auto &iterOutSSBO = *iterOut.get<vkh::ShaderStorage>(0, DescIteration::BINDING_SSBO_ITERATION_MATRIX);
        if (iterOutSSBO.isLocked()) {
            iterOutSSBO.unlock(wc.getCommandPool());
        }
        iterOutSSBO.getHostObject().resizeAndClear<double>(DescIteration::TARGET_SSBO_ITERATION_BUFFER, width * height);
        iterOutSSBO.reloadBuffer();
        iterOutSSBO.lock(wc.getCommandPool());

        const auto &iterOutUBO = *iterOut.get<vkh::Uniform>(0, DescIteration::BINDING_UBO_ITERATION_INFO);
        iterOutUBO.getHostObject().set<glm::uvec2>(DescIteration::TARGET_UBO_ITERATION_EXTENT, {width, height});
        // The video window always renders the whole canvas at once, unlike the tiled still export.
        iterOutUBO.getHostObject().set<glm::uvec2>(DescIteration::TARGET_UBO_ITERATION_CANVAS_EXTENT,
                                                   {width, height});
        iterOutUBO.getHostObject().set<glm::ivec2>(DescIteration::TARGET_UBO_ITERATION_CANVAS_OFFSET, {0, 0});
        iterOutUBO.update(DescIteration::TARGET_UBO_ITERATION_EXTENT);
        iterOutUBO.update(DescIteration::TARGET_UBO_ITERATION_CANVAS_EXTENT);
        iterOutUBO.update(DescIteration::TARGET_UBO_ITERATION_CANVAS_OFFSET);


        writeDescriptorMF([&iter, &iterOut](vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
            iter.queue(queue, frameIndex, {}, {BINDING_I2MAP_SSBO_NORMAL, BINDING_I2MAP_SSBO_ZOOMED});
            iterOut.queue(queue, frameIndex, {}, {DescIteration::BINDING_UBO_ITERATION_INFO, DescIteration::BINDING_SSBO_ITERATION_MATRIX});
        });
    }


    void CPC2MapIterationStripe::setInfo(const double maxIteration, const double normalMaxIteration,
                                         const double zoomedMaxIteration) const {
        using namespace SharedDescriptorTemplate;

        auto &iter = getDescriptor(SET_OUTPUT_ITERATION);
        const auto &iterOutUBO = *iter.get<vkh::Uniform>(0, DescIteration::BINDING_UBO_ITERATION_INFO);
        auto &iterOutUBOHost = iterOutUBO.getHostObject();
        iterOutUBOHost.set<double>(DescIteration::TARGET_UBO_ITERATION_MAX, maxIteration);
        iterOutUBOHost.set<double>(DescIteration::TARGET_UBO_ITERATION_MAX_NORMAL, normalMaxIteration);
        iterOutUBOHost.set<double>(DescIteration::TARGET_UBO_ITERATION_MAX_ZOOMED, zoomedMaxIteration);
        iterOutUBO.update();
    }

    void CPC2MapIterationStripe::advanceAnimationTo(const float sec) {
        // A preview scrubbed to an instant has run nothing up to it, and neither has the first
        // frame of an export; both are re-derived rather than integrated from wherever the phases
        // happened to stand. A frame step is far below the seek threshold and integrates.
        if (phases.isSeek(sec)) {
            phases.seekTo(sec);
        } else {
            phases.advanceTo(sec);
        }
    }

    void CPC2MapIterationStripe::setTime(const float currentSec, const uint32_t frameIndex) {
        using namespace SharedDescriptorTemplate;
        advanceAnimationTo(currentSec);
        auto &time = getDescriptor(SET_TIME);
        const auto &timeUBO = *time.get<vkh::Uniform>(0, DescTime::BINDING_UBO_TIME);
        phases.writeTimeUniform(timeUBO, frameIndex, currentSec);
    }

    void CPC2MapIterationStripe::configurePushConstant(vkh::PipelineLayoutManagerRef pipelineLayoutManager) {
        //noop
    }

    void CPC2MapIterationStripe::configureDescriptors(std::vector<vkh::DescriptorPtr> &descriptors) {
        using namespace SharedDescriptorTemplate;
        auto normal = vkh::factory::create<vkh::HostDataObjectManager>();
        normal->reserveArray<double>(TARGET_I2MAP_SSBO_NORMAL_ITERATION, 1);
        auto normalSSBO = vkh::factory::create<vkh::ShaderStorage>(wc.core, std::move(normal),
                                                                   vkh::BufferLock::ALWAYS_MUTABLE, false);
        auto zoomed = vkh::factory::create<vkh::HostDataObjectManager>();
        zoomed->reserveArray<double>(TARGET_I2MAP_SSBO_ZOOMED_ITERATION, 1);
        auto zoomedSSBO = vkh::factory::create<vkh::ShaderStorage>(wc.core, std::move(zoomed),
                                                                   vkh::BufferLock::ALWAYS_MUTABLE, false);

        auto i2mapManager = vkh::factory::create<vkh::DescriptorManager>();
        i2mapManager->appendSSBO(BINDING_I2MAP_SSBO_NORMAL, VK_SHADER_STAGE_COMPUTE_BIT, std::move(normalSSBO));
        i2mapManager->appendSSBO(BINDING_I2MAP_SSBO_ZOOMED, VK_SHADER_STAGE_COMPUTE_BIT, std::move(zoomedSSBO));
        appendUniqueDescriptor(SET_I2MAP, descriptors, std::move(i2mapManager));
        appendDescriptor<DescVideo>(SET_VIDEO, descriptors);
        appendDescriptor<DescPalette>(SET_PALETTE, descriptors);
        appendDescriptor<DescTime>(SET_TIME, descriptors);

        auto outputManager = vkh::factory::create<vkh::DescriptorManager>();
        outputManager->appendStorageImage(BINDING_OUTPUT_MERGED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
        appendUniqueDescriptor(SET_OUTPUT_IMAGE, descriptors, std::move(outputManager));
        appendDescriptor<DescIteration>(SET_OUTPUT_ITERATION, descriptors);
        appendDescriptor<DescStripe>(SET_STRIPE, descriptors);

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
                               TextureDescriptor::createManager(wc.core, sampler, VK_SHADER_STAGE_COMPUTE_BIT));
    }
}
