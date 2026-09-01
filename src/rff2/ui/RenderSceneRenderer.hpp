//
// Created by Merutilm on 2025-09-05.
// Modified by Opus 5 on 2026-08-10, 2026-08-31
// Modified by GPT-5 on 2026-08-31
//

#pragma once
#include "GpuPassTimer.hpp"
#include "../../vulkan_helper/impl/Renderer.hpp"
#include "../../vulkan_helper/executor/RenderPassFullscreenRecorder.hpp"
#include "../data/GraphicsMatrixStagingBuffer.h"
#include "../vulkan/CPCBoxBlur.hpp"
#include "../vulkan/GPCBloom.hpp"
#include "../vulkan/GPCBloomThreshold.hpp"
#include "../vulkan/GPCColor.hpp"
#include "../vulkan/GPCFog.hpp"
#include "../vulkan/GPCIterationPalette.hpp"
#include "../vulkan/GPCLinearInterpolation.hpp"
#include "../vulkan/GPCDownsampleForBlur.hpp"
#include "../vulkan/GPCPresent.hpp"
#include "../vulkan/GPCSlope.hpp"
#include "../vulkan/GPCStripe.hpp"
#include "../vulkan/RCC0.hpp"
#include "../vulkan/RCC1.hpp"
#include "../vulkan/RCC2.hpp"
#include "../vulkan/RCC3.hpp"
#include "../vulkan/RCC4.hpp"
#include "../vulkan/RCC5.hpp"
#include "../vulkan/RCCDownsampleForBlur.hpp"
#include "../vulkan/RCCPresent.hpp"

namespace merutilm::rff2 {
    struct RenderSceneRenderer final : public vkh::RendererAbstract {

        GPCIterationPalette *rendererIteration = nullptr;
        GPCStripe *rendererStripe = nullptr;
        GPCSlope *rendererSlope = nullptr;
        GPCColor *rendererColor = nullptr;
        GPCDownsampleForBlur *rendererDownsampleForBlur = nullptr;
        CPCBoxBlur *rendererBoxBlur = nullptr;
        GPCFog *rendererFog = nullptr;
        GPCBloomThreshold *rendererBloomThreshold = nullptr;
        GPCBloom *rendererBloom = nullptr;
        GPCLinearInterpolation *rendererLinearInterpolation = nullptr;
        GPCPresent *rendererPresent = nullptr;

        std::unique_ptr<GraphicsMatrixBuffer<double> > iterationStagingBufferContext = nullptr;

        // Off unless the Debug menu turns it on, and pushed in by RenderScene before each frame: the
        // renderer is thrown away and rebuilt on every resize, so the switch cannot live here.
        bool passTimingEnabled = false;
        // One slot per frame in flight. A frame is still on the GPU while the next is being recorded,
        // and its timestamps have to survive that long to be read at all.
        GpuPassTimer passTimer;

        explicit RenderSceneRenderer(vkh::EngineRef engine, const uint32_t windowContextIndex) : RendererAbstract(engine, windowContextIndex),
            passTimer(engine.getCore(), engine.getCore().getPhysicalDevice().getMaxFramesInFlight()) {
            RenderSceneRenderer::init();
        }

        ~RenderSceneRenderer() override {
            RenderSceneRenderer::destroy();
        }

        RenderSceneRenderer(const RenderSceneRenderer &) = delete;

        RenderSceneRenderer &operator=(const RenderSceneRenderer &) = delete;

        RenderSceneRenderer(RenderSceneRenderer &&) = delete;

        RenderSceneRenderer &operator=(RenderSceneRenderer &&) = delete;


    private:
        void init() override {

            rendererIteration = vkh::PipelineConfiguratorAbstract::createShaderProgram<
                GPCIterationPalette>(
                configurators, engine, wc.getAttachmentIndex(),
                RCC0::CONTEXT_INDEX,
                RCC0::SUBPASS_ITERATION_INDEX);

            rendererStripe = vkh::PipelineConfiguratorAbstract::createShaderProgram<GPCStripe>(
                configurators, engine, wc.getAttachmentIndex(),
                RCC1::CONTEXT_INDEX,
                RCC1::SUBPASS_STRIPE_INDEX);

            rendererSlope = vkh::PipelineConfiguratorAbstract::createShaderProgram<GPCSlope>(
                configurators, engine, wc.getAttachmentIndex(),
                RCC2::CONTEXT_INDEX,
                RCC2::SUBPASS_SLOPE_INDEX);

            rendererColor = vkh::PipelineConfiguratorAbstract::createShaderProgram<GPCColor>(
                configurators, engine, wc.getAttachmentIndex(),
                RCC2::CONTEXT_INDEX,
                RCC2::SUBPASS_COLOR_INDEX);


            rendererDownsampleForBlur = vkh::PipelineConfiguratorAbstract::createShaderProgram<GPCDownsampleForBlur>(
                configurators, engine, wc.getAttachmentIndex(),
                RCCDownsampleForBlur::CONTEXT_INDEX,
                RCCDownsampleForBlur::SUBPASS_DOWNSAMPLE_INDEX
            );

            rendererBoxBlur = vkh::PipelineConfiguratorAbstract::createShaderProgram<CPCBoxBlur>(
                configurators, engine, wc.getAttachmentIndex()
            );

            rendererFog = vkh::PipelineConfiguratorAbstract::createShaderProgram<GPCFog>(
                configurators, engine, wc.getAttachmentIndex(),
                RCC3::CONTEXT_INDEX,
                RCC3::SUBPASS_FOG_INDEX
            );

            rendererBloomThreshold = vkh::PipelineConfiguratorAbstract::createShaderProgram<GPCBloomThreshold>(
                configurators, engine, wc.getAttachmentIndex(),
                RCC3::CONTEXT_INDEX,
                RCC3::SUBPASS_BLOOM_THRESHOLD_INDEX
            );

            rendererBloom = vkh::PipelineConfiguratorAbstract::createShaderProgram<GPCBloom>(
                configurators, engine, wc.getAttachmentIndex(),
                RCC4::CONTEXT_INDEX,
                RCC4::SUBPASS_BLOOM_INDEX
            );

            rendererLinearInterpolation = vkh::PipelineConfiguratorAbstract::createShaderProgram<
                GPCLinearInterpolation>(
                configurators, engine, wc.getAttachmentIndex(),
                RCC5::CONTEXT_INDEX,
                RCC5::SUBPASS_LINEAR_INTERPOLATION_INDEX
            );
            rendererPresent = vkh::PipelineConfiguratorAbstract::createShaderProgram<GPCPresent>(
                configurators, engine, wc.getAttachmentIndex(),
                RCCPresent::CONTEXT_INDEX,
                RCCPresent::SUBPASS_PRESENT_INDEX
            );

            finishPipelineInitialization();

        }

        void beforeCmdRender() override {
            // The fence for this frame index has already been waited on, so the marks the last frame
            // at this index wrote are readable now, and the slot they sit in is not reset until the
            // command buffer recorded below actually runs.
            if (passTimingEnabled) {
                passTimer.collect(frameIndex);
            }
            vkh::BufferContext::flush(wc.core.getLogicalDevice().getLogicalDeviceHandle(), iterationStagingBufferContext->getContext());
        }


        void cmdRender(const uint32_t swapchainImageIndex) override {

            const auto cbh = wc.getCommandBuffer().getCommandBufferHandle(frameIndex);
            const auto mfg = [this](const uint32_t index) {
                return wc.getSharedImageContext().getImageContextMF(index)[frameIndex].image;
            };



            // Each mark is one timestamp write, so none is recorded unless the timing was asked for.
            const auto mark = [this, cbh](const char *label) {
                if (passTimingEnabled) {
                    passTimer.cmdMark(cbh, label, frameIndex);
                }
            };
            if (passTimingEnabled) {
                passTimer.cmdReset(cbh, frameIndex);
                mark("start");
            }

            rendererIteration->cmdRefreshIterations(
                wc.getCommandBuffer().getCommandBufferHandle(frameIndex), iterationStagingBufferContext->getContext());

            auto &ctx = rendererIteration->getResultIterationBuffer();
            vkh::BarrierUtils::cmdBufferMemoryBarrier(cbh, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, ctx.buffer, 0, ctx.bufferSize,
                                                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

            // [BARRIER] Safe-copy iteration buffer

            vkh::RenderPassFullscreenRecorder::cmdFullscreenInternalRenderPass<RCC0>(
                wc, frameIndex, {rendererIteration}, {{}});
            mark("iteration");

            // [IN] EXTERNAL
            // [SUBPASS OUT] SECONDARY (iteration)

            vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(cbh,
                                                              mfg(
                                                                  SharedImageContextIndices::MF_MAIN_RENDER_IMAGE_SECONDARY),
                                                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                              0, 1,
                                                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            // [BARRIER] SECONDARY
            vkh::RenderPassFullscreenRecorder::cmdFullscreenInternalRenderPass<RCC1>(
                           wc, frameIndex, {
                               rendererStripe
                           }, {{}});
            mark("stripe");


            // [IN] SECONDARY
            // [OUT] PRIMARY

            vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(cbh,
                                                                          mfg(
                                                                              SharedImageContextIndices::MF_MAIN_RENDER_IMAGE_PRIMARY),
                                                                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                                          0, 1,
                                                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                                          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            // [BARRIER] PRIMARY

            vkh::RenderPassFullscreenRecorder::cmdFullscreenInternalRenderPass<RCC2>(
                wc, frameIndex, {
                    rendererSlope, rendererColor
                }, {{}, {}});
            mark("slope+color");

            // [IN] PRIMARY
            // [SUBPASS OUT] SECONDARY (slope)
            // [SUBPASS IN] SECONDARY
            // [OUT] PRIMARY (color)

            vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(cbh,
                                                              mfg(
                                                                  SharedImageContextIndices::MF_MAIN_RENDER_IMAGE_PRIMARY),
                                                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                              0, 1,
                                                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            // [BARRIER] PRIMARY

            vkh::RenderPassFullscreenRecorder::cmdFullscreenInternalRenderPass<
                RCCDownsampleForBlur>(
                wc, frameIndex, {rendererDownsampleForBlur}, {
                    {GPCDownsampleForBlur::DESC_INDEX_RESAMPLE_IMAGE_FOG}
                });

            // [IN] PRIMARY
            // [OUT] DOWNSAMPLED_PRIMARY

            vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(cbh,
                                                              mfg(
                                                                  SharedImageContextIndices::MF_MAIN_RENDER_DOWNSAMPLED_IMAGE_PRIMARY),
                                                              VK_IMAGE_LAYOUT_GENERAL,
                                                              0, 1,
                                                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                              VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

            // [BARRIER] DOWNSAMPLED_PRIMARY

            rendererBoxBlur->cmdGaussianBlur(frameIndex, CPCBoxBlur::DESC_INDEX_BLUR_TARGET_FOG);
            mark("downsample+blur fog");

            // [IN] DOWNSAMPLED_PRIMARY
            // [OUT] DOWNSAMPLED_SECONDARY

            vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(cbh,
                                                              mfg(
                                                                  SharedImageContextIndices::MF_MAIN_RENDER_IMAGE_PRIMARY),
                                                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                              0, 1,
                                                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            vkh::BarrierUtils::cmdImageMemoryBarrier(
                cbh, mfg(SharedImageContextIndices::MF_MAIN_RENDER_DOWNSAMPLED_IMAGE_SECONDARY),
                VK_ACCESS_SHADER_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            // [BARRIER] PRIMARY
            // [BARRIER] DOWNSAMPLED_SECONDARY

            vkh::RenderPassFullscreenRecorder::cmdFullscreenInternalRenderPass<RCC3>(
                wc, frameIndex, {rendererFog, rendererBloomThreshold}, {{}, {}});
            mark("fog+bloomThreshold");

            // [IN] PRIMARY
            // [IN] DOWNSAMPLED_SECONDARY
            // [PRESERVED SUBPASS OUT] SECONDARY
            // [SUBPASS IN] SECONDARY
            // [OUT] PRIMARY (Threshold Masked)

            vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(cbh,
                                                              mfg(
                                                                  SharedImageContextIndices::MF_MAIN_RENDER_IMAGE_PRIMARY),
                                                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                              0, 1,
                                                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

            // [BARRIER] PRIMARY

            vkh::RenderPassFullscreenRecorder::cmdFullscreenInternalRenderPass<RCCDownsampleForBlur>(
                wc, frameIndex, {rendererDownsampleForBlur}, {
                    {GPCDownsampleForBlur::DESC_INDEX_RESAMPLE_IMAGE_BLOOM}
                });
            // [IN] PRIMARY
            // [OUT] DOWNSAMPLED_PRIMARY

            vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(cbh,
                                                              mfg(
                                                                  SharedImageContextIndices::MF_MAIN_RENDER_DOWNSAMPLED_IMAGE_PRIMARY),
                                                              VK_IMAGE_LAYOUT_GENERAL,
                                                              0, 1,
                                                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                              VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
            // [BARRIER] DOWNSAMPLED_PRIMARY

            rendererBoxBlur->cmdGaussianBlur(frameIndex, CPCBoxBlur::DESC_INDEX_BLUR_TARGET_BLOOM);
            mark("downsample+blur bloom");

            // [IN] DOWNSAMPLED_PRIMARY
            // [OUT] DOWNSAMPLED_SECONDARY

            vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(cbh,
                                                              mfg(
                                                                  SharedImageContextIndices::MF_MAIN_RENDER_IMAGE_SECONDARY),
                                                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                              0, 1,
                                                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            vkh::BarrierUtils::cmdImageMemoryBarrier(
                cbh, mfg(SharedImageContextIndices::MF_MAIN_RENDER_DOWNSAMPLED_IMAGE_SECONDARY),
                VK_ACCESS_SHADER_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

            // [BARRIER] SECONDARY
            // [BARRIER] DOWNSAMPLED_SECONDARY


            vkh::RenderPassFullscreenRecorder::cmdFullscreenInternalRenderPass<RCC4>(
                wc, frameIndex, {rendererBloom}, {{}});
            mark("bloom");

            // [IN] SECONDARY
            // [IN] DOWNSAMPLED_SECONDARY
            // [OUT] PRIMARY

            vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(cbh,
                                                              mfg(
                                                                  SharedImageContextIndices::MF_MAIN_RENDER_IMAGE_PRIMARY),
                                                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                              0, 1,
                                                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

            vkh::RenderPassFullscreenRecorder::cmdFullscreenInternalRenderPass<RCC5>(
                wc, frameIndex, {rendererLinearInterpolation}, {{}});
            mark("linearInterpolation");

            // [IN] PRIMARY
            // [OUT] SECONDARY

            vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(cbh,
                                                              mfg(
                                                                  SharedImageContextIndices::MF_MAIN_RENDER_IMAGE_SECONDARY),
                                                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                              0, 1,
                                                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

            // [BARRIER] SECONDARY

            // An offscreen frame holds no swapchain image, and its caller reads SECONDARY back directly.
            if (offscreenPass) {
                return;
            }

            vkh::RenderPassFullscreenRecorder::cmdFullscreenPresentOnlyRenderPass<RCCPresent>(
                wc, frameIndex, swapchainImageIndex, {rendererPresent}, {{}});
            mark("present");

            // [IN] SECONDARY
            // [OUT] EXTERNAL
        }

        void destroy() override {
            iterationStagingBufferContext = nullptr;
        }
    };
}
