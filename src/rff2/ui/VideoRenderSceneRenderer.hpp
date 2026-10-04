//
// Created by Merutilm on 2025-09-06.
// Modified by Opus 5 on 2026-08-10, 2026-08-19, 2026-08-31
// Modified by GPT-5 on 2026-08-18, 2026-08-23, 2026-08-31
// Modified by GPT-6 on 2026-09-11, 2026-09-16, 2026-09-20, 2026-09-22, 2026-09-23, 2026-09-26
// Modified by Opus 5.5 on 2026-10-04
//

#pragma once
#include "../vulkan/OrderedShaderLayers.hpp"
#include "GpuPassTimer.hpp"
#include "GpuSubmitTimer.hpp"
#include "../../vulkan_helper/configurator/PipelineConfigurator.hpp"
#include "../../vulkan_helper/core/vkh.hpp"
#include "../../vulkan_helper/executor/RenderPassFullscreenRecorder.hpp"
#include "../../vulkan_helper/impl/Renderer.hpp"
#include "../../vulkan_helper/util/BarrierUtils.hpp"
#include "../vulkan/CPCBoxBlur.hpp"
#include "../vulkan/CPC2MapIterationStripe.hpp"
#include "../vulkan/CPCImageRGBA2BGR.hpp"
#include "../vulkan/GPCBloom.hpp"
#include "../vulkan/GPCBloomThreshold.hpp"
#include "../vulkan/GPCColor.hpp"
#include "../vulkan/GPCDownsampleForBlur.hpp"
#include "../vulkan/GPCFog.hpp"
#include "../vulkan/GPCLinearInterpolation.hpp"
#include "../vulkan/GPCPresent.hpp"
#include "../vulkan/GPCSlope.hpp"
#include "../vulkan/GPCStripe.hpp"
#include "../vulkan/GPCStaticImage2Map.hpp"
#include "../vulkan/RCC1Vid.hpp"
#include "../vulkan/RCC2Vid.hpp"
#include "../vulkan/RCC3Vid.hpp"
#include "../vulkan/RCC4Vid.hpp"
#include "../vulkan/RCCDownsampleForBlurVid.hpp"
#include "../vulkan/RCCPresentVid.hpp"
#include "../vulkan/RCCStatic2Image.hpp"
#include "../vulkan/SharedDescriptorTemplate.hpp"

namespace merutilm::rff2 {
    struct VideoRenderSceneRenderer final : public vkh::RendererAbstract {
        GPCStaticImage2Map *rendererStaticImage = nullptr;
        CPC2MapIterationStripe *renderer2MapIterationStripe = nullptr;
        GPCStripe *rendererStripe = nullptr;
        GPCSlope *rendererSlope = nullptr;
        ShaderAttribute layerShader{};
        GPCColor *rendererColor = nullptr;
        GPCDownsampleForBlur *rendererDownsampleForBlur = nullptr;
        CPCBoxBlur *rendererBoxBlur = nullptr;
        GPCFog *rendererFog = nullptr;
        GPCBloomThreshold *rendererBloomThreshold = nullptr;
        GPCBloom *rendererBloom = nullptr;
        GPCLinearInterpolation *rendererLinearInterpolation = nullptr;
        CPCImageRGBA2BGR *rendererImageRGBA2BGR = nullptr;
        GPCPresent *rendererPresent = nullptr;
        bool isStaticImages = false;
        // Picks both the images the chain grades in and the shader variant that writes the first of them.
        bool hdrChain = false;
        double currentSec = 0.0f;
        float currentFrame = 0.0f;
        // 0 sends a frame in one submission; 1 sends each pass on its own; N >= 2 also splits the fractal pass into N bands.
        uint32_t submitSplit = 0;
        GpuPassTimer passTimer;
        GpuSubmitTimer submitTimer;

        explicit VideoRenderSceneRenderer(vkh::EngineRef engine, const uint32_t windowContextIndex,
                                          const bool hdrChain, const std::function<void()>& beforeWait = {},
                                          const ShaderAttribute* shader = nullptr, const bool dither = false) : RendererAbstract(
            engine, windowContextIndex), hdrChain(hdrChain), passTimer(engine.getCore()), submitTimer(engine.getCore()),
            dispatchBaseSupported(engine.getCore().getPhysicalDevice().getPhysicalDeviceProperties().apiVersion >=
                                  VK_API_VERSION_1_1) {
            initialize(beforeWait, shader, dither);
        }

        ~VideoRenderSceneRenderer() override {
            VideoRenderSceneRenderer::destroy();
        }

        VideoRenderSceneRenderer(const VideoRenderSceneRenderer &) = delete;

        VideoRenderSceneRenderer &operator=(const VideoRenderSceneRenderer &) = delete;

        VideoRenderSceneRenderer(VideoRenderSceneRenderer &&) = delete;

        VideoRenderSceneRenderer &operator=(VideoRenderSceneRenderer &&) = delete;

    private:
        const bool dispatchBaseSupported;

        // Ends the submission at a pass boundary when splitting is on, so no submission outlasts the driver's GPU watchdog.
        void splitPoint(const VkCommandBuffer cbh, std::string segmentLabel) {
            if (submitSplit > 0) {
                splitHere(cbh, std::move(segmentLabel));
            }
        }

        void splitHere(const VkCommandBuffer cbh, std::string segmentLabel) {
            submitTimer.cmdEndSegment(cbh, std::move(segmentLabel));
            splitSubmission();
            submitTimer.cmdBeginSegment(cbh);
        }

        void passDone(const VkCommandBuffer cbh, const std::string &label, const std::string &segmentLabel = {}) {
            passTimer.cmdMark(cbh, label);
            splitPoint(cbh, segmentLabel.empty() ? label : segmentLabel);
        }

        // The fractal pass is the one that grows with Supersampling, so it alone is cut further into row bands.
        // Returns the name of the submission the caller closes after it.
        std::string cmd2MapIterationStripe(const VkCommandBuffer cbh) {
            const uint32_t rows = renderer2MapIterationStripe->getWorkGroupRows();
            const uint32_t bands = dispatchBaseSupported ? std::min(submitSplit, rows) : 1u;
            if (bands <= 1) {
                renderer2MapIterationStripe->cmdRender(cbh, frameIndex, {});
                return "2map_iter_stripe";
            }
            for (uint32_t band = 0; band < bands; ++band) {
                if (band > 0) {
                    splitHere(cbh, std::format("2map band {}/{}", band, bands));
                }
                const uint32_t first = static_cast<uint32_t>(uint64_t(rows) * band / bands);
                const uint32_t last = static_cast<uint32_t>(uint64_t(rows) * (band + 1) / bands);
                renderer2MapIterationStripe->cmdRenderRows(cbh, frameIndex, {}, first, last - first);
            }
            return std::format("2map band {}/{}", bands, bands);
        }

        void setLayerStage(int stage) {
            for (auto* pipeline : std::array<vkh::PipelineConfiguratorAbstract*, 7>{renderer2MapIterationStripe, rendererStripe, rendererSlope, rendererColor, rendererFog, rendererBloom, rendererLinearInterpolation})
                ShaderLayerControl::set(*pipeline, stage, layerShader);
        }

        void cmdOrderedLayers() {
            const auto cbh = wc.getCommandBuffer().getCommandBufferHandle(frameIndex);
            const auto mfg = [this](uint32_t index) {
                return wc.getSharedImageContext().getImageContextMF(index)[frameIndex].image;
            };
            const auto &primary = wc.getSharedImageContext().getImageContextMF(
                SharedImageContextIndices::MF_VIDEO_RENDER_IMAGE_PRIMARY)[frameIndex];
            const auto &secondary = wc.getSharedImageContext().getImageContextMF(
                SharedImageContextIndices::MF_VIDEO_RENDER_IMAGE_SECONDARY)[frameIndex];
            const auto surfacePass = [&] {
                vkh::RenderPassFullscreenRecorder::cmdFullscreenInternalRenderPass<RCC1Vid>(
                    wc, frameIndex, {rendererSlope, rendererColor}, {{}, {}});
                vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(
                    cbh, primary.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            };
            const auto fogBloomPass = [&] {
                vkh::RenderPassFullscreenRecorder::cmdFullscreenInternalRenderPass<RCCDownsampleForBlurVid>(
                    wc, frameIndex, {rendererDownsampleForBlur},
                    {{GPCDownsampleForBlur::DESC_INDEX_RESAMPLE_IMAGE_FOG}});

                vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(
                    cbh, mfg(SharedImageContextIndices::MF_VIDEO_RENDER_DOWNSAMPLED_IMAGE_PRIMARY),
                    VK_IMAGE_LAYOUT_GENERAL, 0, 1, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

                rendererBoxBlur->cmdGaussianBlur(frameIndex, CPCBoxBlur::DESC_INDEX_BLUR_TARGET_FOG);
                passDone(cbh, "downsample+blur fog");

                vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(
                    cbh, mfg(SharedImageContextIndices::MF_VIDEO_RENDER_IMAGE_PRIMARY),
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
                vkh::BarrierUtils::cmdImageMemoryBarrier(
                    cbh, mfg(SharedImageContextIndices::MF_VIDEO_RENDER_DOWNSAMPLED_IMAGE_SECONDARY),
                    VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

                vkh::RenderPassFullscreenRecorder::cmdFullscreenInternalRenderPass<RCC2Vid>(
                    wc, frameIndex, {rendererFog, rendererBloomThreshold}, {{}, {}});
                passDone(cbh, "fog+bloomThreshold");

                vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(
                    cbh, mfg(SharedImageContextIndices::MF_VIDEO_RENDER_IMAGE_PRIMARY),
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

                vkh::RenderPassFullscreenRecorder::cmdFullscreenInternalRenderPass<RCCDownsampleForBlurVid>(
                    wc, frameIndex, {rendererDownsampleForBlur},
                    {{GPCDownsampleForBlur::DESC_INDEX_RESAMPLE_IMAGE_BLOOM}});

                vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(
                    cbh, mfg(SharedImageContextIndices::MF_VIDEO_RENDER_DOWNSAMPLED_IMAGE_PRIMARY),
                    VK_IMAGE_LAYOUT_GENERAL, 0, 1, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

                rendererBoxBlur->cmdGaussianBlur(frameIndex, CPCBoxBlur::DESC_INDEX_BLUR_TARGET_BLOOM);
                passDone(cbh, "downsample+blur bloom");

                vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(
                    cbh, mfg(SharedImageContextIndices::MF_VIDEO_RENDER_IMAGE_SECONDARY),
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
                vkh::BarrierUtils::cmdImageMemoryBarrier(
                    cbh, mfg(SharedImageContextIndices::MF_VIDEO_RENDER_DOWNSAMPLED_IMAGE_SECONDARY),
                    VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

                vkh::RenderPassFullscreenRecorder::cmdFullscreenInternalRenderPass<RCC3Vid>(
                    wc, frameIndex, {rendererBloom}, {{}});
                passDone(cbh, "bloom");

                vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(
                    cbh, mfg(SharedImageContextIndices::MF_VIDEO_RENDER_IMAGE_PRIMARY),
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            };
            setLayerStage(ShaderLayerControl::INITIALIZE);
            surfacePass();
            splitPoint(cbh, "layer surface");
            for (const auto layer : layerShader.layerOrder.layers) {
                if (!shaderLayerActive(layerShader, layer)) {
                    continue;
                }
                if (isStaticImages && uint32_t(layer) <= 22) {
                    continue;
                }
                setLayerStage(int(layer));
                if (layer == ShdLayer::STRIPE) {
                    copyShaderLayerImage(cbh, primary, secondary);
                    vkh::RenderPassFullscreenRecorder::cmdFullscreenInternalRenderPass<RCC3Vid>(
                        wc, frameIndex, {rendererStripe}, {{}});
                } else if (layer == ShdLayer::FOG || layer == ShdLayer::BLOOM) {
                    fogBloomPass();
                } else if (uint32_t(layer) >= 26) {
                    vkh::RenderPassFullscreenRecorder::cmdFullscreenInternalRenderPass<RCC4Vid>(
                        wc, frameIndex, {rendererLinearInterpolation}, {{}});
                    copyShaderLayerImage(cbh, secondary, primary);
                } else {
                    surfacePass();
                }
                vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(
                    cbh, primary.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
                splitPoint(cbh, std::format("layer {}", uint32_t(layer)));
            }
            setLayerStage(ShaderLayerControl::FINALIZE);
        }

        void init() override { initialize({}, nullptr, false); }

        void initialize(const std::function<void()>& beforeWait, const ShaderAttribute* shader, bool dither) {
            vkh::PipelinePreparation::Batch preparation;
            rendererStaticImage = vkh::PipelineConfiguratorAbstract::createShaderProgram<GPCStaticImage2Map>(
                configurators, engine, wc.getAttachmentIndex(),
                RCCStatic2Image::CONTEXT_INDEX,
                RCCStatic2Image::SUBPASS_STATIC_IMAGE_INDEX);

            renderer2MapIterationStripe = vkh::PipelineConfiguratorAbstract::createShaderProgram<
                CPC2MapIterationStripe>(
                configurators, engine, wc.getAttachmentIndex(), hdrChain, shader, dither);

            rendererSlope = vkh::PipelineConfiguratorAbstract::createShaderProgram<GPCSlope>(
                configurators, engine, wc.getAttachmentIndex(),
                RCC1Vid::CONTEXT_INDEX,
                RCC1Vid::SUBPASS_SLOPE_INDEX, renderer2MapIterationStripe->getDescriptor(CPC2MapIterationStripe::SET_TEXTURE));

            rendererColor = vkh::PipelineConfiguratorAbstract::createShaderProgram<GPCColor>(
                configurators, engine, wc.getAttachmentIndex(),
                RCC1Vid::CONTEXT_INDEX,
                RCC1Vid::SUBPASS_COLOR_INDEX);

            rendererDownsampleForBlur = vkh::PipelineConfiguratorAbstract::createShaderProgram<GPCDownsampleForBlur>(
                configurators, engine, wc.getAttachmentIndex(),
                RCCDownsampleForBlurVid::CONTEXT_INDEX,
                RCCDownsampleForBlurVid::SUBPASS_DOWNSAMPLE_INDEX
            );

            rendererBoxBlur = vkh::PipelineConfiguratorAbstract::createShaderProgram<CPCBoxBlur>(
                configurators, engine, wc.getAttachmentIndex()
            );

            rendererFog = vkh::PipelineConfiguratorAbstract::createShaderProgram<GPCFog>(
                configurators, engine, wc.getAttachmentIndex(),
                RCC2Vid::CONTEXT_INDEX,
                RCC2Vid::SUBPASS_FOG_INDEX
            );

            rendererBloomThreshold = vkh::PipelineConfiguratorAbstract::createShaderProgram<GPCBloomThreshold>(
                configurators, engine, wc.getAttachmentIndex(),
                RCC2Vid::CONTEXT_INDEX,
                RCC2Vid::SUBPASS_BLOOM_THRESHOLD_INDEX
            );

            rendererBloom = vkh::PipelineConfiguratorAbstract::createShaderProgram<GPCBloom>(
                configurators, engine, wc.getAttachmentIndex(),
                RCC3Vid::CONTEXT_INDEX,
                RCC3Vid::SUBPASS_BLOOM_INDEX
            );

            rendererLinearInterpolation = vkh::PipelineConfiguratorAbstract::createShaderProgram<
                GPCLinearInterpolation>(
                configurators, engine, wc.getAttachmentIndex(),
                RCC4Vid::CONTEXT_INDEX,
                RCC4Vid::SUBPASS_LINEAR_INTERPOLATION_INDEX
            );
            rendererImageRGBA2BGR = vkh::PipelineConfiguratorAbstract::createShaderProgram<CPCImageRGBA2BGR>(
                configurators, engine, wc.getAttachmentIndex()
            );
            rendererPresent = vkh::PipelineConfiguratorAbstract::createShaderProgram<GPCPresent>(
                configurators, engine, wc.getAttachmentIndex(),
                RCCPresentVid::CONTEXT_INDEX,
                RCCPresentVid::SUBPASS_PRESENT_INDEX
            );
            rendererStripe = vkh::PipelineConfiguratorAbstract::createShaderProgram<GPCStripe>(configurators, engine, wc.getAttachmentIndex(), RCC3Vid::CONTEXT_INDEX, RCC3Vid::SUBPASS_BLOOM_INDEX);
            if (beforeWait) beforeWait();
            preparation.finish();
            finishPipelineInitialization();
        }


        void beforeCmdRender() override {
            setLayerStage(layerShader.layerOrder.enabled ? ShaderLayerControl::INITIALIZE : 0);
            renderer2MapIterationStripe->setTime(currentSec, frameIndex);
            renderer2MapIterationStripe->setCurrentFrame(currentFrame, frameIndex);
        }


        // Everything that grades the picture standing in PRIMARY, from the relief the light is
        // cast on down to the bloom. Both sources arrive here with a finished picture in that
        // image, so a PNG keyframe is graded by the very passes a computed one is. What a PNG
        // cannot carry is the iteration data the stripe and the relief are read from, and those
        // are held off in the attribute rather than here (VideoRenderScene::staticGradeBase).
        void cmdGradeChain() {
            if (layerShader.layerOrder.enabled) { cmdOrderedLayers(); return; }
            const auto cbh = wc.getCommandBuffer().getCommandBufferHandle(frameIndex);
            const auto mfg = [this](const uint32_t index) {
                return wc.getSharedImageContext().getImageContextMF(index)[frameIndex].image;
            };
            vkh::RenderPassFullscreenRecorder::cmdFullscreenInternalRenderPass<RCC1Vid>(
                wc, frameIndex, {
                    rendererSlope,
                    rendererColor
                }, {{}, {}});
            passDone(cbh, "slope+color");

            // [IN] SSBO (Iteration Buffer)
            // [IN] SECONDARY
            // [SUBPASS OUT] PRIMARY (stripe)
            // [SUBPASS IN] PRIMARY
            // [SUBPASS OUT] SECONDARY (slope)
            // [SUBPASS IN] SECONDARY
            // [OUT] PRIMARY (color)

            vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(cbh,
                                                              mfg(
                                                                  SharedImageContextIndices::MF_VIDEO_RENDER_IMAGE_PRIMARY),
                                                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                              0, 1,
                                                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            // [BARRIER] PRIMARY

            vkh::RenderPassFullscreenRecorder::cmdFullscreenInternalRenderPass<
                RCCDownsampleForBlurVid>(
                wc, frameIndex, {rendererDownsampleForBlur}, {
                    {GPCDownsampleForBlur::DESC_INDEX_RESAMPLE_IMAGE_FOG}
                });

            // [IN] PRIMARY
            // [OUT] DOWNSAMPLED_PRIMARY

            vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(cbh,
                                                              mfg(
                                                                  SharedImageContextIndices::MF_VIDEO_RENDER_DOWNSAMPLED_IMAGE_PRIMARY),
                                                              VK_IMAGE_LAYOUT_GENERAL,
                                                              0, 1,
                                                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                              VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

            // [BARRIER] DOWNSAMPLED_PRIMARY

            rendererBoxBlur->cmdGaussianBlur(frameIndex, CPCBoxBlur::DESC_INDEX_BLUR_TARGET_FOG);
            passDone(cbh, "downsample+blur fog");

            // [IN] DOWNSAMPLED_PRIMARY
            // [OUT] DOWNSAMPLED_SECONDARY

            vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(cbh,
                                                              mfg(
                                                                  SharedImageContextIndices::MF_VIDEO_RENDER_IMAGE_PRIMARY),
                                                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                              0, 1,
                                                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            vkh::BarrierUtils::cmdImageMemoryBarrier(
                cbh, mfg(SharedImageContextIndices::MF_VIDEO_RENDER_DOWNSAMPLED_IMAGE_SECONDARY),
                VK_ACCESS_SHADER_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            // [BARRIER] PRIMARY
            // [BARRIER] DOWNSAMPLED_SECONDARY

            vkh::RenderPassFullscreenRecorder::cmdFullscreenInternalRenderPass<RCC2Vid>(
                wc, frameIndex, {rendererFog, rendererBloomThreshold}, {{}, {}});
            passDone(cbh, "fog+bloomThreshold");

            // [IN] PRIMARY
            // [IN] DOWNSAMPLED_SECONDARY
            // [PRESERVED SUBPASS OUT] SECONDARY
            // [SUBPASS IN] SECONDARY
            // [OUT] PRIMARY (Threshold Masked)

            vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(cbh,
                                                              mfg(
                                                                  SharedImageContextIndices::MF_VIDEO_RENDER_IMAGE_PRIMARY),
                                                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                              0, 1,
                                                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

            // [BARRIER] PRIMARY

            vkh::RenderPassFullscreenRecorder::cmdFullscreenInternalRenderPass<RCCDownsampleForBlurVid>(
                wc, frameIndex, {rendererDownsampleForBlur}, {
                    {GPCDownsampleForBlur::DESC_INDEX_RESAMPLE_IMAGE_BLOOM}
                });
            // [IN] PRIMARY
            // [OUT] DOWNSAMPLED_PRIMARY

            vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(cbh,
                                                              mfg(
                                                                  SharedImageContextIndices::MF_VIDEO_RENDER_DOWNSAMPLED_IMAGE_PRIMARY),
                                                              VK_IMAGE_LAYOUT_GENERAL,
                                                              0, 1,
                                                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                              VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
            // [BARRIER] DOWNSAMPLED_PRIMARY

            rendererBoxBlur->cmdGaussianBlur(frameIndex, CPCBoxBlur::DESC_INDEX_BLUR_TARGET_BLOOM);
            passDone(cbh, "downsample+blur bloom");

            // [IN] DOWNSAMPLED_PRIMARY
            // [OUT] DOWNSAMPLED_SECONDARY

            vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(cbh,
                                                              mfg(
                                                                  SharedImageContextIndices::MF_VIDEO_RENDER_IMAGE_SECONDARY),
                                                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                              0, 1,
                                                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            vkh::BarrierUtils::cmdImageMemoryBarrier(
                cbh, mfg(SharedImageContextIndices::MF_VIDEO_RENDER_DOWNSAMPLED_IMAGE_SECONDARY),
                VK_ACCESS_SHADER_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

            // [BARRIER] SECONDARY
            // [BARRIER] DOWNSAMPLED_SECONDARY


            vkh::RenderPassFullscreenRecorder::cmdFullscreenInternalRenderPass<RCC3Vid>(
                wc, frameIndex, {rendererBloom}, {{}});
            passDone(cbh, "bloom");

            // [IN] SECONDARY
            // [IN] DOWNSAMPLED_SECONDARY
            // [OUT] PRIMARY

            vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(cbh,
                                                              mfg(
                                                                  SharedImageContextIndices::MF_VIDEO_RENDER_IMAGE_PRIMARY),
                                                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                              0, 1,
                                                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        }
        void cmdRender(const uint32_t swapchainImageIndex) override {
            const auto cbh = wc.getCommandBuffer().getCommandBufferHandle(frameIndex);
            const auto mfg = [this](const uint32_t index) {
                return wc.getSharedImageContext().getImageContextMF(index)[frameIndex].image;
            };
            passTimer.cmdReset(cbh);
            passTimer.cmdMark(cbh, "start");
            submitTimer.cmdBeginFrame(cbh);
            if (isStaticImages) {
                vkh::RenderPassFullscreenRecorder::cmdFullscreenInternalRenderPass<RCCStatic2Image>(
                    wc, frameIndex, {
                        rendererStaticImage
                    }, {{}});

                vkh::BarrierUtils::cmdImageMemoryBarrier(
                  cbh, mfg(SharedImageContextIndices::MF_VIDEO_RENDER_IMAGE_PRIMARY),
                  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                  VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL,
                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1,
                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
                passDone(cbh, "static image");
                cmdGradeChain();
            } else {
                vkh::BarrierUtils::cmdImageMemoryBarrier(
                    cbh, mfg(SharedImageContextIndices::MF_VIDEO_RENDER_IMAGE_PRIMARY),
                    0, VK_ACCESS_SHADER_WRITE_BIT,
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
                    0, 1,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
                // [BARRIER] Init image

                const std::string fractalSegment = cmd2MapIterationStripe(cbh);
                passDone(cbh, "2map_iter_stripe", fractalSegment);

                // [IN] EXTERNAL
                // [OUT] SSBO (Iteration Buffer)
                // [OUT] PRIMARY

                const auto &outputBuffer = renderer2MapIterationStripe->getDescriptor(
                            CPC2MapIterationStripe::SET_OUTPUT_ITERATION).get
                        <vkh::ShaderStorage>(0, SharedDescriptorTemplate::DescIteration::BINDING_SSBO_ITERATION_MATRIX)
                        ->
                        getBufferContext();

                vkh::BarrierUtils::cmdBufferMemoryBarrier(cbh, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                                                          outputBuffer.buffer, 0, outputBuffer.bufferSize,
                                                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
                vkh::BarrierUtils::cmdImageMemoryBarrier(
                    cbh, mfg(SharedImageContextIndices::MF_VIDEO_RENDER_IMAGE_PRIMARY),
                    VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                    VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    0, 1,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

                // [BARRIER] SSBO (Result Iteration Buffer)
                // [BARRIER] PRIMARY (Result Image)

                cmdGradeChain();

            }

            vkh::RenderPassFullscreenRecorder::cmdFullscreenInternalRenderPass<RCC4Vid>(
                wc, frameIndex, {rendererLinearInterpolation}, {{}});
            passDone(cbh, "linearInterpolation");

            // [IN] PRIMARY
            // [OUT] SECONDARY

            vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(cbh,
                                                          mfg(
                                                              SharedImageContextIndices::MF_VIDEO_RENDER_IMAGE_SECONDARY),
                                                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                          0, 1,
                                                          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);



            // [BARRIER] SECONDARY

            rendererImageRGBA2BGR->cmdRender(cbh, frameIndex, {});
            passTimer.cmdMark(cbh, "rgba2bgr+downsample");

            if (!offscreenPass) {
                vkh::RenderPassFullscreenRecorder::cmdFullscreenPresentOnlyRenderPass<RCCPresentVid>(
                    wc, frameIndex, swapchainImageIndex, {rendererPresent}, {{}});
                passTimer.cmdMark(cbh, "present (preview)");
            }
            submitTimer.cmdEndSegment(cbh, submitSplit == 0 ? "whole frame"
                                           : offscreenPass ? "rgba2bgr+downsample" : "rgba2bgr+present");



            // [IN] SECONDARY
            // [OUT] EXTERNAL
        }

        void destroy() override {
            //noop
        }
    };
}
